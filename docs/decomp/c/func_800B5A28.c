/*
 * func_800B5A28
 *
 * Purpose:
 *   Extracts the pointer at offset +0x10 of `arg0` and passes it to
 *   func_800AEB28 (GPU driver dispatch wrapper), returning its result.
 *
 * Arguments:
 *   arg0 (void *): struct pointer containing a sub-descriptor pointer at +0x10
 *
 * Return value:
 *   signed int: return value forwarded from func_800AEB28
 */

#define W(p, o) (*(unsigned int *)((unsigned char *)(p) + (o)))

extern signed int func_800AEB28(void *arg0);

signed int func_800B5A28(void *arg0)
{
    /* UNCLEAR: exact struct layout of arg0 and meaning of field +0x10 */
    return func_800AEB28((void *)W(arg0, 0x10));
}
