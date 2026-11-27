/*
 * func_800B6F30 (and 3 trailing unlabelled CD-ROM utility functions)
 *
 * Purpose:
 *   func_800B6F30: Thin wrapper that calls func_800B8550() and returns its result.
 *
 *   Three unlabelled trailing functions follow `jr $ra` inside func_800B6F30.s:
 *   1. CdSetDebug equivalent: sets D_800D3958 = level and returns previous value.
 *   2. CdComstr equivalent: returns command name string D_800D3974[com] for
 *      (com & 0xFF) < 0x1C, or fallback string D_8001517C.
 *   3. CdIntstr equivalent: returns interrupt name string D_800D39F4[intr] for
 *      (intr & 0xFF) < 7, or fallback string D_8001517C.
 *
 * Arguments:
 *   func_800B6F30: None (void; or args passed through to func_800B8550)
 *
 * Return value:
 *   func_800B6F30: signed int return value of func_800B8550()
 *
 * Globals read / written:
 *   D_800D3958 (s32, R/W): CD-ROM debug/print verbosity level
 *   D_800D3974 (char *[], R): CD-ROM command name table (28 entries)
 *   D_800D39F4 (char *[], R): CD-ROM interrupt name table (7 entries)
 *   D_8001517C (char[], R): fallback string for out-of-range codes
 */

extern signed int D_800D3958;
extern char *D_800D3974[];
extern char *D_800D39F4[];
extern char D_8001517C[];

extern signed int func_800B8550(void);

signed int func_800B6F30(void)
{
    return func_800B8550();
}

/* UNCLEAR: unlabelled function #1 following jr $ra in func_800B6F30.s (CdSetDebug) */
signed int sub_800B6F30_set_debug(signed int level)
{
    signed int prev = D_800D3958;
    D_800D3958 = level;
    return prev;
}

/* UNCLEAR: unlabelled function #2 following jr $ra in func_800B6F30.s (CdComstr) */
char *sub_800B6F30_comstr(unsigned char com)
{
    unsigned int idx = (unsigned int)com & 0xFFU;
    if (idx < 0x1CU) {
        return D_800D3974[idx];
    }
    return D_8001517C;
}

/* UNCLEAR: unlabelled function #3 following jr $ra in func_800B6F30.s (CdIntstr) */
char *sub_800B6F30_intstr(unsigned char intr)
{
    unsigned int idx = (unsigned int)intr & 0xFFU;
    if (idx < 7U) {
        return D_800D39F4[idx];
    }
    return D_8001517C;
}
