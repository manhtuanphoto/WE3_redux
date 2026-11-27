/*
 * func_800AF820
 *
 * Purpose:
 *   Builds a PS1 GPU GP0(0xE3) "Set Drawing Area Top-Left" command word.
 *   Clamps signed 16-bit `x` to [0, D_800CEAC8 - 1] and `y` to [0, D_800CEACA - 1],
 *   then packs 0xE3000000 | ((y & 0x3FF) << 10) | (x & 0x3FF).
 *
 * Arguments:
 *   x (signed short): drawing area left X coordinate
 *   y (signed short): drawing area top Y coordinate
 *
 * Return value:
 *   unsigned int: packed GP0(0xE3) GPU command word
 *
 * Globals read:
 *   D_800CEAC8 (s16/u16): maximum VRAM/drawing width (1024)
 *   D_800CEACA (s16/u16): maximum VRAM/drawing height (512)
 */

extern signed short D_800CEAC8;
extern signed short D_800CEACA;

unsigned int func_800AF820(signed short x, signed short y)
{
    signed int cx;
    signed int cy;

    if (x < 0) {
        cx = 0;
    } else if (x > (signed int)D_800CEAC8 - 1) {
        cx = (signed int)((unsigned short)D_800CEAC8 - 1);
    } else {
        cx = (signed int)x;
    }

    if (y < 0) {
        cy = 0;
    } else if (y > (signed int)D_800CEACA - 1) {
        cy = (signed int)((unsigned short)D_800CEACA - 1);
    } else {
        cy = (signed int)y;
    }

    return 0xE3000000U | (((unsigned int)cy & 0x3FFU) << 10) | ((unsigned int)cx & 0x3FFU);
}
