/*
 * func_800C1E78
 *
 * Purpose:
 *   Forwards `arg0` as the second argument to func_800BA798 with channel/index 4
 *   (SPU DMA channel 4 in PS1 conventions) and returns its result.
 *
 * Arguments:
 *   arg0 (unsigned int / void *): callback or parameter forwarded to func_800BA798
 *
 * Return value:
 *   int: return value forwarded from func_800BA798
 */

extern int func_800BA798(int channel, unsigned int arg0);

int func_800C1E78(unsigned int arg0)
{
    /* UNCLEAR: exact behavior of func_800BA798 (likely SPU DMA channel 4 callback/config) */
    return func_800BA798(4, arg0);
}
