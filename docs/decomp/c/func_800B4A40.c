/*
 * func_800B4A40
 *
 * Purpose:
 *   Transforms the base light matrix D_800F0EC0 by rotation matrix `m` and loads
 *   the result into the PS1 GTE Light Matrix (analogous to PsyQ GsSetLightMatrix
 *   / light matrix update). Copies the 32-byte MATRIX D_800F0EC0 to a local
 *   stack buffer, saves the active GTE rotation matrix via func_800B4FA8
 *   (PushMatrix), multiplies `local_mtx = local_mtx * m` via func_800B50F8,
 *   restores the GTE rotation matrix via func_800B504C (PopMatrix), and uploads
 *   `local_mtx` to GTE light matrix registers $8..$12 via func_800B5208.
 *
 * Arguments:
 *   m (const signed short *): 3x3 rotation matrix (MATRIX *) applied to D_800F0EC0
 *
 * Return value:
 *   None (void)
 *
 * Globals read:
 *   D_800F0EC0 (unsigned int[8]): base 32-byte MATRIX (light matrix template)
 */

extern unsigned int D_800F0EC0[8];

extern void func_800B4FA8(void);
extern signed short *func_800B50F8(signed short *m0, const signed short *m1);
extern void func_800B504C(void);
extern void func_800B5208(const unsigned int *mat);

void func_800B4A40(const signed short *m)
{
    unsigned int local_mtx[8];
    int i;

    for (i = 0; i < 8; i++) {
        local_mtx[i] = D_800F0EC0[i];
    }

    func_800B4FA8();
    func_800B50F8((signed short *)local_mtx, m);
    func_800B504C();
    func_800B5208(local_mtx);
}
