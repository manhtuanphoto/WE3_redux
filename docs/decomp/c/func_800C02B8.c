/*
 * func_800C02B8
 *
 * Purpose:
 *   Updates left/right MIDI/SEQ channel volume (clamped to 0x7F) on the track
 *   struct selected by packedId, then scans active SPU voices (0..D_800FB578-1)
 *   matching that sequence/channel and recomputes their squared stereo SPU
 *   volumes in D_8010A418/D_8010A41A from expression, master, track, program,
 *   and 3-stage pan/mono settings, flagging D_800E81E0 with |= 3.
 *
 * Arguments:
 *   packedId (int)          - low byte = seq index, high byte = channel/track index,
 *                             low s16 matched against voice seq ID
 *   volL     (unsigned int) - target left volume (clamped to 0x7F)
 *   volR     (unsigned int) - target right volume (clamped to 0x7F)
 *
 * Return value:
 *   (signed short)packedId
 *
 * Reads / Writes:
 *   Globals:
 *     D_8010C2A8: pointer table to sequence channel arrays (stride 176 bytes/track) (R)
 *       track+0x26 (s8) channel id (R)
 *       track+0x58 (u16) volL, +0x5A (u16) volR (W/R)
 *       track+0x60 (s16[]) expression table (R)
 *     D_800FB578 (s8): active voice count (R)
 *     D_800D4F0C (u32): free/masked voice bitmask (R)
 *     D_800E78E0..D_800E78F0: active voice array (54-byte stride) (R)
 *     D_800F20AC (u8*): master audio config (+0x18 u8 master volume) (R)
 *     D_800F1098 (u8*): track table (16-byte stride: +0x01 vol, +0x04 pan) (R)
 *     D_800F20B0 (u8*): tone/program table (32-byte stride: +0x02 vol, +0x03 pan) (R)
 *     D_800F0F70 (s16): mono flag (1 = mono) (R)
 *     D_8010A418 / D_8010A41A (u16, 16-byte stride): SPU left/right volume (W)
 *     D_800E81E0 (u8[]): SPU dirty flags (|= 3) (W)
 *
 * Magic numbers:
 *   0x7F   = 127 max MIDI volume/pan
 *   0x3FFF = 16383 max PS1 SPU voice volume
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define SB(p,o) (*(signed char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned int D_8010C2A8[];
extern unsigned char D_800FB578[];
extern unsigned int D_800D4F0C;
extern unsigned char D_800E78E0[];
extern unsigned char D_800E78E2[];
extern unsigned char D_800E78E4[];
extern unsigned char D_800E78E8[];
extern unsigned char D_800E78EA[];
extern unsigned char D_800E78EC[];
extern unsigned char D_800E78EE[];
extern unsigned char D_800E78F0[];
extern unsigned char* D_800F20AC;
extern unsigned char* D_800F1098;
extern unsigned char* D_800F20B0;
extern signed short D_800F0F70;
extern unsigned char D_8010A418[];
extern unsigned char D_8010A41A[];
extern unsigned char D_800E81E0[];

extern void func_800C0AB8(int, int);

int func_800C02B8(int packedId, unsigned int volL, unsigned int volR) {
    unsigned char* seqBase = (unsigned char*)D_8010C2A8[packedId & 0xFF];
    unsigned int trackIdx = (unsigned int)((packedId & 0xFF00) >> 8);
    unsigned char* track = seqBase + trackIdx * 176;
    int i;

    H(track, 0x58) = (unsigned short)volL;
    H(track, 0x5A) = (unsigned short)volR;
    if (H(track, 0x58) >= 0x7F) {
        H(track, 0x58) = 0x7F;
    }
    if (H(track, 0x5A) >= 0x7F) {
        H(track, 0x5A) = 0x7F;
    }

    for (i = 0; (signed short)i < SB(D_800FB578, 0); i++) {
        int voiceOff;
        int chan;
        int prog;
        int expr;
        int scaled;
        unsigned int baseVol;
        unsigned int outL;
        unsigned int outR;
        unsigned char* toneEntry;
        unsigned int pan;

        if (D_800D4F0C & (1u << (signed short)i)) {
            continue;
        }

        voiceOff = (signed short)i * 54;
        if (S(D_800E78E8, voiceOff) != (signed short)packedId) {
            continue;
        }
        chan = S(D_800E78F0, voiceOff);
        if (chan != SB(track, 0x26)) {
            continue;
        }

        prog = S(D_800E78EA, voiceOff);
        func_800C0AB8(chan, prog);

        expr = (S(D_800E78E0, voiceOff) * S(track, 0x60 + S(D_800E78E4, voiceOff) * 2)) / 127;
        scaled = (16383 * expr * B(D_800F20AC, 0x18)) / 16129;

        toneEntry = D_800F20B0 + ((S(D_800E78EA, voiceOff) << 4) + S(D_800E78EE, voiceOff)) * 32;
        baseVol = (unsigned int)(scaled * B(D_800F1098, (S(D_800E78EC, voiceOff) << 4) + 1) * B(toneEntry, 2)) / 16129u;

        outL = ((unsigned int)(baseVol * H(track, 0x58))) / 127u;
        outR = ((unsigned int)(baseVol * H(track, 0x5A))) / 127u;

        pan = B(toneEntry, 3);
        if (pan < 0x40) {
            outR = ((unsigned int)((int)outR * (int)pan)) / 63u;
        } else {
            outL = ((unsigned int)((int)outL * (int)(0x7F - pan))) / 63u;
        }

        pan = B(D_800F1098, (S(D_800E78EC, (signed short)i * 54) << 4) + 4);
        if (pan < 0x40) {
            outR = ((outR & 0xFFFFu) * pan) / 63u;
        } else {
            outL = (unsigned int)((int)(outL & 0xFFFFu) * (int)(0x7F - pan)) / 63;
        }

        pan = B(D_800E78E2, (signed short)i * 54);
        if (pan < 0x40) {
            outR = ((outR & 0xFFFFu) * pan) / 63u;
        } else {
            outL = (unsigned int)((int)(outL & 0xFFFFu) * (int)(0x7F - pan)) / 63;
        }

        if (D_800F0F70 == 1) {
            if ((outL & 0xFFFFu) < (outR & 0xFFFFu)) {
                outL = outR;
            } else {
                outR = outL;
            }
        }

        outL &= 0xFFFFu;
        outR &= 0xFFFFu;
        H(D_8010A418, (signed short)i * 16) = (unsigned short)((int)(outL * outL) / 16383);
        H(D_8010A41A, (signed short)i * 16) = (unsigned short)((int)(outR * outR) / 16383);
        B(D_800E81E0, (signed short)i) |= 3;
    }

    return (signed short)packedId;
}
