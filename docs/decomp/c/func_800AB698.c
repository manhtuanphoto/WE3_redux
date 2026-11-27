/*
 * func_800AB698 - Menu / match-flow command dispatcher (cases 1..17)
 *
 * Purpose:
 *   Updates D_800E6B82 = D_800E6B76 - 0x100 and dispatches command IDs 1..17
 *   for UI/pause/replay/sound-channel/state transitions, audio fades, and
 *   table resets.
 *
 * Arguments:
 *   cmd (unsigned int) - command index (1..17 handled; others no-op)
 *
 * Return value:
 *   None (void).
 *
 * Global fields read & written:
 *   D_800E6B76 (u16/s16) - current menu/submode ID (cleared in cases 1, 14, 15)
 *   D_800E6B82 (s16)     - D_800E6B76 - 0x100 (written on entry)
 *   D_800E6BD8 (u8)      - active overlay/dialog toggle flag
 *   D_800E6B4C (u16/s16) - screen state (3/4 -> 0x3C/0x44, 0x3C/0x3D -> 0x3E, 0x44/0x47 -> 0x46)
 *   D_800E6B6E (s16)     - flag selecting -1 vs D_800E6B76 for func_800A920C
 *   D_800E6AA0 (s16)     - argument passed to func_800A97A0
 *   D_800E6B14 (u8)      - sound/BGM flag (1 in case 4, 0 in case 5)
 *   D_800E6B15 (u8)      - sound/SE flag (1 in case 3, 0 in cases 4, 5)
 *   D_800E6B16 (u16)     - sub-selection set to 1, 2, 3, 8 in cases 6..9
 *   D_800E6BD6 (u16)     - audio mode (0 in case 4, 4 in case 5)
 *   D_800E6B74 (u8)      - toggle flag (1 in case 12, 0 in case 13)
 *   D_800E6B84 (s16)     - cleared in case 15 and case 16/17 (if D_800E6BCC < 2000)
 *   D_800E6AC8 (u8)      - cleared in case 14/15
 *   D_800E6BCC (s32)     - thresholded against 2000 (0x7D0) in case 16/17
 *   D_800E6A98 / D_800E6A9A (s16) - cleared in case 16/17
 *   D_800CE7F6 (s16[])   - 32 halfwords zeroed backwards in case 16/17
 *   D_800CE845..D_800CE847, D_800CE868..D_800CE86D, D_800EF89C, D_800EF89E - UI/layout tables
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x100 (256)  = offset subtracted from D_800E6B76 into D_800E6B82.
 *   0x200 (512)  = threshold separating submode branch vs screen-state branch in case 2.
 *   0x7D0 (2000) = D_800E6BCC threshold in case 16/17.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned short D_800E6B76;
extern signed short D_800E6B82;
extern unsigned char D_800E6BD8;
extern unsigned short D_800E6B4C;
extern signed short D_800E6B6E;
extern signed short D_800E6AA0;
extern unsigned char D_800E6B14;
extern unsigned char D_800E6B15;
extern unsigned short D_800E6B16;
extern unsigned short D_800E6BD6;
extern unsigned char D_800E6B74;
extern signed short D_800E6B84;
extern unsigned char D_800E6AC8;
extern signed int D_800E6BCC;
extern signed short D_800E6A98;
extern signed short D_800E6A9A;
extern signed short D_800EF89C;
extern signed short D_800EF89E;
extern unsigned char D_800CE7F6[];
extern unsigned char D_800CE845[];
extern unsigned char D_800CE846[];
extern unsigned char D_800CE868[];
extern unsigned char D_800CE86A[];
extern unsigned char D_800CE86C[];

extern void func_800AAC90(void);
extern void func_800AAC48(void);
extern void func_800BDFA4(int a0, int a1, int a2);
extern void func_800BCEB8(int x, int y);
extern void func_800BCEF8(int x, int y, int w, int h);
extern void func_800BD040(int a0, int a1, int a2, int a3);
extern void func_800AB620(int id);
extern void func_800A97A0(int a0);
extern void func_800A9394(int a0);
extern unsigned short func_800A920C(int a0);
extern void func_800AC074(void);
extern void func_800C0B78(unsigned char *buf);
extern void func_800C0B88(unsigned char *buf);
extern void func_800B73F0(unsigned char *buf);
extern void func_800ABB94(void);
extern void func_800BD328(int a0);
extern void func_800C3ED8(int a0);

void func_800AB698(unsigned int cmd)
{
    unsigned char buf[4];

    D_800E6B82 = (signed short)(D_800E6B76 - 0x100);

    switch (cmd) {
    case 1:
        func_800AAC90();
        func_800AAC48();
        D_800E6BD8 = 0;
        D_800E6B76 = 0;
        break;

    case 2:
        if (D_800E6BD8 == 0) {
            int idx1, idx2;
            D_800E6BD8 = 1;
            /* UNCLEAR: asm puts `ori $a0, $zero, 0x16` in the delay slot of
             * `jal func_800AAC48` and does not reload $a0 before func_800BDFA4. */
            func_800AAC48();
            func_800BDFA4(0x16, 0, 0);

            idx1 = D_800EF89E;
            func_800BCEB8(S(D_800CE868, idx1 * 8), S(D_800CE86A, idx1 * 8));
            idx2 = D_800EF89C;
            func_800BCEB8(S(D_800CE868, idx2 * 8), S(D_800CE86A, idx2 * 8));
            func_800BCEF8(S(D_800CE868, 0), S(D_800CE86A, 0), 0x5F, 0x5F);

            if (D_800E6B76 < 0x200) {
                if (D_800E6B76 != 0) {
                    int off = (unsigned short)D_800E6B82 << 5;
                    func_800BD040(0, B(D_800CE845, off), 0x32, 0x32);
                }
            } else {
                if ((unsigned short)(D_800E6B4C - 3) < 2) {
                    D_800E6B4C = ((signed short)D_800E6B4C == 3) ? 0x3C : 0x44;
                }
            }
            func_800AB620(0x513);
        } else {
            int idx;
            func_800A97A0(D_800E6AA0);
            func_800BCEB8(S(D_800CE868, 0), S(D_800CE86A, 0));
            idx = D_800EF89E;
            func_800BCEF8(S(D_800CE868, idx * 8), S(D_800CE86A, idx * 8),
                          B(D_800CE86C, idx * 8), B(D_800CE86C, idx * 8 + 1));

            if (D_800E6B76 < 0x200) {
                if (D_800E6B76 != 0) {
                    int off = (unsigned short)D_800E6B82 << 5;
                    func_800BD040(0, B(D_800CE845, off),
                                  B(D_800CE846, off), B(D_800CE846, off + 1));
                }
            } else {
                if ((unsigned short)(D_800E6B4C - 0x3C) < 2) {
                    func_800A9394(1);
                    D_800E6B4C = 0x3E;
                }
                if ((signed short)D_800E6B4C == 0x44 || (signed short)D_800E6B4C == 0x47) {
                    int arg = (D_800E6B6E == 0) ? -1 : (int)D_800E6B76;
                    D_800E6B4C = func_800A920C(arg);
                }
            }
            D_800E6BD8 = 0;
            func_800AC074();
        }
        break;

    case 3:
        D_800E6B15 = 1;
        break;

    case 4:
        D_800E6B14 = 1;
        D_800E6B15 = 0;
        D_800E6BD6 = 0;
        func_800C0B78(buf);
        buf[0] = 0x50;
        buf[1] = 0x50;
        buf[2] = 0x50;
        buf[3] = 0x50;
        func_800B73F0(buf);
        break;

    case 5:
        D_800E6B14 = 0;
        D_800E6B15 = 0;
        D_800E6BD6 = 4;
        func_800C0B88(buf);
        buf[0] = 0x7F;
        buf[1] = 0;
        buf[2] = 0x7F;
        buf[3] = 0;
        func_800B73F0(buf);
        break;

    case 6:
        if ((unsigned short)(D_800E6B4C - 3) < 2) {
            D_800E6B16 = 1;
        }
        break;

    case 7:
        if ((unsigned short)(D_800E6B4C - 3) < 2) {
            D_800E6B16 = 2;
        }
        break;

    case 8:
        if ((unsigned short)(D_800E6B4C - 3) < 2) {
            D_800E6B16 = 3;
        }
        break;

    case 9:
        if ((unsigned short)(D_800E6B4C - 3) < 2) {
            D_800E6B16 = 8;
        }
        break;

    case 10:
        func_800ABB94();
        break;

    case 11:
        break;

    case 12:
        D_800E6B74 = 1;
        break;

    case 13:
        D_800E6B74 = 0;
        break;

    case 15:
        D_800E6B84 = 0;
        func_800AAC48();
        func_800BD328(0);
        /* fall through */
    case 14:
        func_800ABB94();
        D_800E6B76 = 0;
        func_800AAC90();
        D_800E6BD8 = 0;
        D_800E6AC8 = 0;
        break;

    case 17:
        func_800C3ED8(0);
        func_800AAC48();
        /* fall through */
    case 16: {
        int i;
        signed short *p = (signed short *)D_800CE7F6;
        for (i = 31; i >= 0; i--) {
            *p-- = 0;
        }
        D_800E6A98 = 0;
        D_800E6A9A = 0;
        if (D_800E6BCC < 2000) {
            D_800E6B84 = 0;
        }
        break;
    }

    default:
        break;
    }
}
