/*
 * func_800C5A24 (plus an adjacent unlabeled function in func_800C5A24.s)
 *
 * Purpose:
 *   PS1 Memory Card / BIOS control wrappers:
 *   1. func_800C5A24: Enters critical section (func_800AD3D8), invokes
 *      func_800C5A98(), calls func_800ACF08(0), and flushes instruction cache
 *      (func_800ACE68).
 *   2. func_800C5A50 [unlabeled at +0x2C]: Calls func_800C5AA8() and
 *      func_800C5C88().
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 */

extern void func_800AD3D8(void);
extern void func_800C5A98(void);
extern void func_800ACF08(int disable);
extern void func_800ACE68(void);
extern void func_800C5AA8(void);
extern void func_800C5C88(void);

void func_800C5A24(void)
{
    func_800AD3D8();
    func_800C5A98();
    func_800ACF08(0);
    func_800ACE68();
}

/* UNCLEAR: unlabeled second function inside func_800C5A24.s (at 0x800C5A50) */
void func_800C5A50(void)
{
    func_800C5AA8();
    func_800C5C88();
}
