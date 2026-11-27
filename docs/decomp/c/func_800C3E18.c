/*
 * func_800C3E18
 *
 * Purpose:
 *   Stores `mode` to D_800D511C and stores a normalized boolean (1 if mode == 1,
 *   else 0) to D_800D55A8, returning that normalized value.
 *
 * Arguments:
 *   mode (int): Mode/flag value (0, 1, or other)
 *
 * Return value:
 *   int: 1 if mode == 1, otherwise 0
 *
 * Globals written:
 *   D_800D511C (s32)
 *   D_800D55A8 (s32)
 */

extern int D_800D511C;
extern int D_800D55A8;

int func_800C3E18(int mode)
{
    int normalized;

    if (mode == 0) {
        normalized = 0;
    } else if (mode == 1) {
        normalized = 1;
    } else {
        normalized = 0;
    }

    D_800D511C = mode;
    D_800D55A8 = normalized;
    return normalized;
}
