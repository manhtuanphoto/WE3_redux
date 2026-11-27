/*
 * func_800B4D04
 *
 * Purpose:
 *   Reads three 32-bit fields at offsets +0x14, +0x18, and +0x1C from `arg0`
 *   (matching the translation vector `t[3]` of a PsyQ MATRIX struct) and
 *   prints them via func_800AD698 (BIOS A(0x3F) printf) using format string
 *   D_800150A0.
 *
 * Arguments:
 *   arg0 (const unsigned char *): Pointer to struct (likely MATRIX *) with
 *                                 32-bit fields at +0x14, +0x18, +0x1C
 *
 * Return value:
 *   int: Return value from func_800AD698 (printf)
 *
 * Globals read:
 *   D_800150A0 (const char *): Format string
 */

extern const char D_800150A0[];

extern int func_800AD698(const char *fmt, int v0, int v1, int v2);

int func_800B4D04(const unsigned char *arg0)
{
    int x = *(const int *)(arg0 + 0x14);
    int y = *(const int *)(arg0 + 0x18);
    int z = *(const int *)(arg0 + 0x1C);

    return func_800AD698(D_800150A0, x, y, z);
}
