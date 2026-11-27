/*
 * func_800C3008 - Configure XA/CD/SPU audio channel parameters and volume/pitch
 *
 * Purpose:
 *   Applies an audio parameter command packet (`cmd`):
 *   - flags == 0 updates all fields; otherwise bitmask bits select:
 *     - bit 0 (0x01): select track/channel index (cmd+0x04, bit 0x100 triggers
 *       func_800C39E8), validate (< 10) and check readiness via func_800C2F78,
 *       load 68-byte config template D_800D564C[idx].
 *     - bit 3 (0x08): update pitch/tempo fields for channels 7..8 from cmd+0x0C
 *       (scaled by 8192/127 and 4096/127).
 *     - bit 4 (0x10): update master volume field for channels 7..8 from cmd+0x10
 *       (scaled by 33024/127).
 *     - bit 1 (0x02) / bit 2 (0x04): update left/right CD/SPU volumes at
 *       D_800D558C+0x184/+0x186 (cleared when channel is switched).
 *   Submits updated 68-byte channel config via func_800C34E8 and starts playback
 *   via func_800C1C44(0xD1, ...) if a new channel was selected.
 *
 * Arguments:
 *   cmd (unsigned char *) - parameter struct (+0x00 flags, +0x04 channel,
 *                           +0x08 left_vol, +0x0A right_vol, +0x0C pitch, +0x10 vol)
 *
 * Return value:
 *   0 on success, -1 if channel index >= 10 or func_800C2F78 fails.
 *
 * Global fields read & written:
 *   D_800D5128 (u32)  - active channel table value from D_800D55FC[idx]
 *   D_800D5130 (s32)  - active channel index (0..9)
 *   D_800D5134 (u16)  - cached left volume
 *   D_800D5136 (u16)  - cached right volume
 *   D_800D5138 (s32)  - cached pitch value
 *   D_800D513C (s32)  - cached volume value
 *   D_800D55FC (u32[10])  - channel handles/IDs
 *   D_800D564C (u8[10][68]) - 68-byte channel parameter templates
 *   D_800D558C (u8 *) - audio state struct (+0x184/+0x186 vol, +0x1AA bit 7 mute/pause)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   127 (0x7F) = max MIDI/PS1 audio parameter range (divisors for pitch/volume).
 *   0x0C011C00 / 0x80 = dirty-field masks in the 68-byte config struct.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned int D_800D5128;
extern signed int D_800D5130;
extern unsigned short D_800D5134;
extern unsigned short D_800D5136;
extern signed int D_800D5138;
extern signed int D_800D513C;
extern unsigned char *D_800D558C;
extern unsigned int D_800D55FC[];
extern unsigned char D_800D564C[];

extern int func_800C2F78(unsigned int handle);
extern void func_800C34E8(unsigned char *cfg);
extern void func_800C39E8(int channel);
extern void func_800C1C44(int cmd, unsigned int handle, int arg2);

int func_800C3008(unsigned char *cmd)
{
    unsigned char cfg[68];
    int flag_100 = 0;
    int saved_mute = 0;
    int did_chan = 0;
    int did_pitch = 0;
    int did_vol = 0;
    unsigned int flags = W(cmd, 0x00);
    int update_all = (flags < 1);

    W(cfg, 0x00) = 0;

    if (update_all || (flags & 0x01)) {
        unsigned int chan = W(cmd, 0x04);
        int i;
        unsigned char *src;

        if (chan & 0x100) {
            chan &= ~0x100u;
            flag_100 = 1;
        }
        if (chan >= 10) {
            return -1;
        }
        if (func_800C2F78(D_800D55FC[chan]) != 0) {
            return -1;
        }

        did_chan = 1;
        D_800D5130 = (signed int)chan;
        D_800D5128 = D_800D55FC[chan];

        src = D_800D564C + chan * 68;
        for (i = 0; i < 68; i++) {
            cfg[i] = src[i];
        }

        if (D_800D5130 == 7) {
            D_800D513C = 0x7F;
            D_800D5138 = 0x7F;
        } else if (D_800D5130 == 8) {
            D_800D513C = 0;
            D_800D5138 = 0x7F;
        } else {
            D_800D513C = 0;
            D_800D5138 = 0;
        }
    }

    if (update_all || (flags & 0x08)) {
        if (D_800D5130 >= 7 && D_800D5130 < 9) {
            signed int val;
            signed int scaled_hi;
            signed int scaled_lo;

            did_pitch = 1;
            if (!did_chan) {
                unsigned char *src = D_800D564C + D_800D5130 * 68;
                int i;
                for (i = 0; i < 68; i++) {
                    cfg[i] = src[i];
                }
                W(cfg, 0x00) = 0x0C011C00;
            }

            val = SW(cmd, 0x0C);
            D_800D5138 = val;
            scaled_hi = (val << 13) / 127;
            scaled_lo = (val << 12) / 127;

            H(cfg, 0x18) = (unsigned short)(scaled_hi - H(cfg, 0x04));
            H(cfg, 0x1A) = (unsigned short)(scaled_lo - H(cfg, 0x06));
            H(cfg, 0x24) = (unsigned short)(H(cfg, 0x26) + scaled_lo);
            H(cfg, 0x1C) = (unsigned short)(H(cfg, 0x1E) + scaled_lo);
            H(cfg, 0x3A) = (unsigned short)(H(cfg, 0x3E) + scaled_lo);
            H(cfg, 0x38) = (unsigned short)(H(cfg, 0x3C) + scaled_lo);
        }
    }

    if (update_all || (flags & 0x10)) {
        if (D_800D5130 >= 7 && D_800D5130 < 9) {
            signed int val;

            did_vol = 1;
            if (!did_chan) {
                if (!did_pitch) {
                    unsigned char *src = D_800D564C + D_800D5130 * 68;
                    int i;
                    for (i = 0; i < 68; i++) {
                        cfg[i] = src[i];
                    }
                    W(cfg, 0x00) = 0x80;
                } else {
                    W(cfg, 0x00) |= 0x80;
                }
            }

            val = SW(cmd, 0x10);
            D_800D513C = val;
            H(cfg, 0x12) = (unsigned short)((val * 33024) / 127);
        }
    }

    if (did_chan) {
        unsigned char *st = D_800D558C;
        saved_mute = (H(st, 0x1AA) >> 7) & 1;
        if (saved_mute) {
            H(st, 0x1AA) &= 0xFF7F;
        }
        H(D_800D558C, 0x184) = 0;
        H(D_800D558C, 0x186) = 0;
        D_800D5134 = 0;
        D_800D5136 = 0;
    } else {
        if (update_all || (flags & 0x02)) {
            H(D_800D558C, 0x184) = H(cmd, 0x08);
            D_800D5134 = H(cmd, 0x08);
        }
        if (update_all || (flags & 0x04)) {
            H(D_800D558C, 0x186) = H(cmd, 0x0A);
            D_800D5136 = H(cmd, 0x0A);
        }
    }

    if (did_chan || did_pitch || did_vol) {
        func_800C34E8(cfg);
    }

    if (flag_100) {
        func_800C39E8(D_800D5130);
    }

    if (did_chan) {
        func_800C1C44(0xD1, D_800D5128, 0);
        if (saved_mute) {
            H(D_800D558C, 0x1AA) |= 0x80;
        }
    }

    return 0;
}
