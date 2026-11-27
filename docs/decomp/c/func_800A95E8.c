/*
 * func_800A95E8
 *
 * Purpose:
 *   Clears D_800E6B6A, issues CD/command 9 with argument 0 via func_800B7170
 *   (likely CdlPause), clears D_800E6B28, and returns 0.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   int: Always returns 0
 *
 * Globals written:
 *   D_800E6B6A (s16/u16)
 *   D_800E6B28 (s16/u16)
 */

extern unsigned short D_800E6B6A;
extern unsigned short D_800E6B28;

extern int func_800B7170(unsigned char cmd, int arg);

int func_800A95E8(void)
{
    D_800E6B6A = 0;
    func_800B7170(9, 0);
    D_800E6B28 = 0;
    return 0;
}
