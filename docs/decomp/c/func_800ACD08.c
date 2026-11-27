/*
 * func_800ACD08
 *
 * Purpose:
 *   CD-ROM / XA audio playback timeout watchdog and state transition handler.
 *   Polls func_800B6FCC(1, 0) (CdSync-style status); if status == 2 (complete),
 *   resets frame counter D_800E6B00 to 0. Otherwise increments D_800E6B00, and
 *   once it reaches 21 (0x15) frames, steps state variable D_800E6B4C through
 *   recovery/restart transitions and resets D_800E6B00 to 0.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800E6B00 (unsigned char): timeout frame counter (triggers at >= 0x15 = 21)
 *   D_800E6B4C (signed short): CD/XA audio state (8->0x14, 6->7, 7->func_800AC074,
 *                              0x1E->0x28, 0x28->1, 0x5C/0x14->func_800B7034)
 *   D_800E6AF0 (unsigned char[]): buffer passed to func_800B7034(1, 0, D_800E6AF0)
 */

extern unsigned char D_800E6B00;
extern signed short D_800E6B4C;
extern unsigned char D_800E6AF0[];

extern unsigned int func_800B6FCC(unsigned int mode, unsigned int param);
extern void func_800AC074(void);
extern void func_800B7034(unsigned int a0, unsigned int a1, unsigned char *buf);

void func_800ACD08(void)
{
    if (func_800B6FCC(1u, 0u) == 2u) {
        D_800E6B00 = 0;
        return;
    }

    D_800E6B00 = (unsigned char)(D_800E6B00 + 1);
    if (D_800E6B00 < 0x15u) {
        return;
    }

    /* Sequential checks allow state changes to chain within the same tick */
    if (D_800E6B4C == 8) {
        D_800E6B4C = 0x14;
    }
    if (D_800E6B4C == 6) {
        D_800E6B4C = 7;
    }
    if (D_800E6B4C == 7) {
        func_800AC074();
    }
    if (D_800E6B4C == 0x1E) {
        D_800E6B4C = 0x28;
    }
    if (D_800E6B4C == 0x28) {
        D_800E6B4C = 1;
    }
    if (D_800E6B4C == 0x5C) {
        func_800B7034(1u, 0u, D_800E6AF0);
    }
    if (D_800E6B4C == 0x14) {
        func_800B7034(1u, 0u, D_800E6AF0);
    }

    D_800E6B00 = 0;
}
