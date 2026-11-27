/*
 * func_800A91E4
 *
 * Purpose:
 *   Thin wrapper that calls func_800B6FEC(1, D_800E6AF8).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   (int) return value forwarded from func_800B6FEC
 *   UNCLEAR: func_800B6FEC is not in asm_compact; exact return type and
 *   meaning of D_800E6AF8 are unverified (D_800E6AF8 sits near CD param
 *   buffer D_800E6AE0 used by func_800A95C0).
 *
 * Global fields referenced:
 *   D_800E6AF8 (u8[])
 */

extern unsigned char D_800E6AF8[];

extern int func_800B6FEC(int arg0, unsigned char *arg1);

int func_800A91E4(void)
{
    return func_800B6FEC(1, D_800E6AF8);
}
