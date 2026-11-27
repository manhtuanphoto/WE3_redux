/*
 * func_800BEC88 - Sound/SPU voice allocation and note/SFX trigger.
 *
 * Purpose:
 *   Allocates SPU voice channels for a sound/instrument trigger, populates the
 *   global voice parameter table (54-byte entries starting at D_800E78D8) and
 *   command state (D_8010A3CD..D_8010A3E2), and triggers hardware voice playback
 *   via func_800BE2B8 / func_800BF758 / func_800BFDE8 (or delegates to
 *   func_800BF1FC when arg4 == 0).
 *
 * Arguments:
 *   arg0 (int): sound/program ID (low byte = bank table index, high byte = sub-bank).
 *   arg1 (int): pitch/note or priority parameter (signed 16-bit).
 *   arg2 (int): entry index into D_800F1098 (16-byte entries, signed 16-bit).
 *   arg3 (int): pan/channel attribute byte.
 *   arg4 (int): volume/velocity (unsigned 16-bit; 0 delegates to func_800BF1FC).
 *   arg5 (int): flag/mode byte (unsigned 16-bit, stored at D_8010A3CD).
 *
 * Return value:
 *   int: bitmask of triggered voices, or -1 on validation/channel-limit failure.
 *
 * Scratchpad reads/writes:
 *   - None.
 *
 * Actor fields reads/writes:
 *   - None (sound engine globals D_8010A3CD..D_8010A3E2, D_800E78D8..D_800E78F5).
 *
 * Magic numbers:
 *   - 0x21 (33) = special sound ID using raw arg4 volume instead of scaled / 127.
 *   - 127 = MIDI-style velocity scaling divisor.
 *   - 0xB0 (176) = sub-bank stride in bank data pointer table D_8010C2A8.
 *   - 54 (0x36) = SPU voice state struct stride at D_800E78D8.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char *D_8010C2A8[];
extern unsigned char D_8010A3CD[];
extern unsigned char D_8010A3DE[];
extern unsigned char D_8010A3E2[];
extern unsigned char *D_800F1098;
extern unsigned char *D_800F20AC;
extern unsigned char *D_800F20B0;
extern signed char D_800FB578;

extern unsigned char D_800E78D8[];
extern unsigned char D_800E78DA[];
extern unsigned char D_800E78E0[];
extern unsigned char D_800E78E2[];
extern unsigned char D_800E78E4[];
extern unsigned char D_800E78E6[];
extern unsigned char D_800E78E8[];
extern unsigned char D_800E78EA[];
extern unsigned char D_800E78EC[];
extern unsigned char D_800E78EE[];
extern unsigned char D_800E78F0[];
extern unsigned char D_800E78F2[];
extern unsigned char D_800E78F5[];

extern int func_800C0AB8(int arg1, int arg2);
extern int func_800BF1FC(int arg0, int arg1, int arg2, int arg3);
extern int func_800C09E8(unsigned char *buf1, unsigned char *buf2);
extern int func_800BE028(int arg0);
extern void func_800BE2B8(void);
extern void func_800BF758(int ch);
extern int func_800BF498(int count);
extern void func_800BFDE8(int unused_a0, int val);

int func_800BEC88(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5)
{
    unsigned char buf_10[128];
    unsigned char buf_90[128];
    int result = 0;
    int s1_id;
    int bank_idx;
    unsigned char *bank_ptr;
    unsigned short vol;
    unsigned short mode_flag;
    unsigned char *cmd_base;
    unsigned char *entry_ptr;
    int count;
    int i;

    s1_id = (signed short)arg0;
    bank_idx = (s1_id & 0xFF00) >> 8;
    bank_ptr = D_8010C2A8[arg0 & 0xFF] + bank_idx * 0xB0;
    vol = (unsigned short)arg4;
    mode_flag = (unsigned short)arg5;

    if (func_800C0AB8((signed short)arg1, (signed short)arg2) != 0) {
        return -1;
    }

    H(D_8010A3DE, 0) = (unsigned short)arg0;
    B(D_8010A3DE, -0x14) = (unsigned char)arg3;
    B(D_8010A3DE, -0x13) = 0;
    if (s1_id == 0x21) {
        B(D_8010A3DE, -0x12) = (unsigned char)vol;
    } else {
        int scaled = (int)vol * S(bank_ptr, 0x60 + B(bank_ptr, 0x17) * 2);
        B(D_8010A3DE, -0x12) = (unsigned char)(scaled / 127);
    }

    cmd_base = D_8010A3CD;
    B(cmd_base, 0) = (unsigned char)mode_flag;

    entry_ptr = D_800F1098 + ((signed short)arg2 << 4);
    B(cmd_base, 5) = B(entry_ptr, 1);
    B(cmd_base, 6) = B(entry_ptr, 4);
    B(cmd_base, -5) = B(entry_ptr, 0);

    if ((signed char)B(cmd_base, 2) >= (int)H(D_800F20AC, 0x12)) {
        return -1;
    }

    if (vol == 0) {
        return func_800BF1FC((signed short)arg0, (signed short)arg1, (signed short)arg2, arg3 & 0xFFFF);
    }

    count = func_800C09E8(buf_90, buf_10) & 0xFF;
    if (count == 0) {
        return 0;
    }

    cmd_base += 0x13;
    for (i = 0; i < count; i++) {
        int idx;
        unsigned char *tone_ptr;
        int ch;
        int stride;

        H(cmd_base, 0) = buf_10[i];
        B(cmd_base, -0xC) = buf_90[i];
        idx = ((int)(signed char)buf_90[i] + ((int)(signed char)B(cmd_base, -0x11) << 4)) & 0xFFFF;
        tone_ptr = D_800F20B0 + (idx << 5);

        B(cmd_base, -9) = B(tone_ptr, 0);
        B(cmd_base, -0xB) = B(tone_ptr, 2);
        B(cmd_base, -0xA) = B(tone_ptr, 3);
        B(cmd_base, -8) = B(tone_ptr, 4);
        B(cmd_base, -7) = B(tone_ptr, 5);
        B(cmd_base, -4) = B(tone_ptr, 1);
        B(cmd_base, -6) = B(tone_ptr, 6);
        B(cmd_base, -5) = B(tone_ptr, 7);

        ch = func_800BE028(0) & 0xFF;
        H(cmd_base, 2) = (unsigned short)ch;
        if (ch < (int)D_800FB578) {
            stride = ch * 54;
            B(D_800E78F5, stride) = 1;
            H(D_800E78DA, stride) = 0;
            H(D_800E78E8, stride) = (unsigned short)arg0;
            H(D_800E78F0, stride) = (unsigned short)(signed short)(signed char)B(cmd_base, -0x17);
            H(D_800E78EA, stride) = (unsigned short)(signed short)(signed char)B(cmd_base, -0x11);
            H(D_800E78EC, stride) = (unsigned short)arg2;
            if ((signed short)arg0 != 0x21) {
                H(D_800E78E0, stride) = vol;
                H(D_800E78E4, stride) = B(bank_ptr, 0x17);
            }
            B(D_800E78E2, stride) = (unsigned char)mode_flag;
            H(D_800E78EE, stride) = (unsigned short)(signed short)(signed char)B(cmd_base, -0xC);
            H(D_800E78E6, stride) = (unsigned short)arg3;
            H(D_800E78F2, stride) = (unsigned short)(signed short)(signed char)B(cmd_base, -9);
            H(D_800E78D8, stride) = H(cmd_base, 0);

            func_800BE2B8();
            if (S(cmd_base, 0) == 0xFF) {
                func_800BF758(B(cmd_base, 2));
            } else {
                int ret = func_800BF498(count);
                /* UNCLEAR: asm does not reload $a0 after func_800BF498; func_800BFDE8 may use only $a1 */
                func_800BFDE8(count, ret & 0xFFFF);
            }
            result |= (1 << S(D_8010A3E2, 0));
        } else {
            result = -1;
        }
    }

    return result;
}
