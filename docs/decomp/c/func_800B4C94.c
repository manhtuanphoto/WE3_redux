/*
 * func_800B4C94
 *
 * Purpose:
 *   Debug print helper (DumpMatrix-style). Reads 9 consecutive signed 16-bit
 *   integers (offsets +0x00..+0x10, e.g., a 3x3 MATRIX) and passes them to
 *   func_800AD698() with format string D_8001507C.
 *
 * Arguments:
 *   m (const signed short *): pointer to 9 signed 16-bit values
 *
 * Return value:
 *   int: return value forwarded from func_800AD698() (if any)
 *
 * Global fields read:
 *   D_8001507C (u8[]): format string passed to func_800AD698
 */

extern unsigned char D_8001507C[];

extern int func_800AD698(
    unsigned char *fmt,
    int m0, int m1, int m2,
    int m3, int m4, int m5,
    int m6, int m7, int m8
);

int func_800B4C94(const signed short *m)
{
    return func_800AD698(
        D_8001507C,
        (int)m[0], (int)m[1], (int)m[2],
        (int)m[3], (int)m[4], (int)m[5],
        (int)m[6], (int)m[7], (int)m[8]
    );
}
