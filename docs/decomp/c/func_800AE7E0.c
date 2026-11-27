/*
 * func_800AE7E0
 *
 * Purpose:
 *   Invokes func_800AE59C(D_80014F54, a0) and then dispatches an indirect call
 *   through the function pointer at *(D_800CEABC + 0x08), passing
 *   *(D_800CEABC + 0x20), a0, 8, and a1.
 *
 * Arguments:
 *   a0 (unsigned int): passed to func_800AE59C and as arg1 to D_800CEABC->fn_08
 *   a1 (unsigned int): passed as arg3 to D_800CEABC->fn_08
 *
 * Return value:
 *   int: return value from D_800CEABC->fn_08
 *
 * Global fields read:
 *   D_80014F54 (u8[]): label/format string passed to func_800AE59C
 *   D_800CEABC (u8 *): descriptor/ops struct (+0x08 fn ptr, +0x20 handle/arg0)
 */

#define W(p, o) (*(unsigned int *)((p) + (o)))

typedef int (*func_800AE7E0_cb)(unsigned int, unsigned int, unsigned int, unsigned int);

extern unsigned char D_80014F54[];
extern unsigned char *D_800CEABC;

extern void func_800AE59C(unsigned char *tag, unsigned int a0);

int func_800AE7E0(unsigned int a0, unsigned int a1)
{
    unsigned char *ctx;
    func_800AE7E0_cb fn;

    func_800AE59C(D_80014F54, a0);
    ctx = D_800CEABC;
    fn = (func_800AE7E0_cb)W(ctx, 0x08);
    return fn(W(ctx, 0x20), a0, 8u, a1);
}
