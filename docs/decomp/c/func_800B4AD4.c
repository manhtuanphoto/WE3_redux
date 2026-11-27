/*
 * func_800B4AD4
 *
 * Purpose:
 *   Multiplies the global matrix D_800F0EC0 by `mat` in-place via
 *   func_800B11B8() (GTE MulMatrix) and loads the resulting matrix into the
 *   GTE light matrix registers ($8-$12) via func_800B5208().
 *
 * Arguments:
 *   mat (unsigned int *): Pointer to 3x3 matrix (modified in-place by func_800B11B8)
 *
 * Return value:
 *   None (void)
 *
 * Globals read:
 *   D_800F0EC0 (unsigned int[5] / MATRIX)
 */

extern const unsigned int D_800F0EC0[];

extern unsigned int *func_800B11B8(const unsigned int *m0, unsigned int *m1);
extern void func_800B5208(const unsigned int *mat);

void func_800B4AD4(unsigned int *mat)
{
    unsigned int *result = func_800B11B8(D_800F0EC0, mat);
    func_800B5208(result);
}
