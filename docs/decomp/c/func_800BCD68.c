/*
 * func_800BCD68
 *
 * Purpose:
 *   Configures the PS1 libsnd tick mode and tick frequency globals
 *   (analogous to PsyQ SsSetTickMode), querying video mode (0 = NTSC, 1 = PAL)
 *   via func_800BB22C and setting D_800D4F50, D_800D4F4C, and D_8010C1BC.
 *
 * Arguments:
 *   tickMode: (int) tick mode selector (bit 0x1000 = flag stored in D_800D4F50)
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   D_800D4F4C: (s32) active tick mode (R/W)
 *   D_800D4F50: (s32) 1 if (tickMode & 0x1000), else 0 (W)
 *   D_8010C1BC: (s32) tick rate in Hz (e.g. 50, 60, 120, 240, or custom) (W)
 */

extern int D_800D4F4C;
extern int D_800D4F50;
extern int D_8010C1BC;

extern int func_800BB22C(int arg);

void func_800BCD68(int tickMode) {
    int vidMode = func_800BB22C(tickMode);
    int mode;

    if ((tickMode & 0x1000) != 0) {
        D_800D4F50 = 1;
        D_800D4F4C = tickMode & 0xFFF;
    } else {
        D_800D4F50 = 0;
        D_800D4F4C = tickMode;
    }

    mode = D_800D4F4C;
    if (mode >= 6) {
        D_8010C1BC = mode;
        return;
    }
    if (mode < 0) {
        D_8010C1BC = 0x3C; /* 60 */
        return;
    }

    switch (mode) {
    case 0:
    case 5:
        if (vidMode == 0) {
            D_8010C1BC = 0x3C;
        } else if (vidMode == 1) {
            D_8010C1BC = 0x32;
        } else {
            D_8010C1BC = 0x3C;
        }
        break;
    case 1:
        D_8010C1BC = 0x3C; /* 60 */
        if (vidMode == 0) {
            D_800D4F4C = 5;
        } else {
            D_800D4F4C = 0x3C;
        }
        break;
    case 2:
        D_8010C1BC = 0xF0; /* 240 */
        break;
    case 3:
        D_8010C1BC = 0x78; /* 120 */
        break;
    case 4:
        D_8010C1BC = 0x32; /* 50 */
        if (vidMode == 1) {
            D_800D4F4C = 5;
        } else {
            D_800D4F4C = 0x32;
        }
        break;
    }
}
