/*
 * func_800AD13C
 *
 * Purpose:
 *   Performs a subsystem initialization/setup sequence: runs helper steps
 *   func_800AD588..func_800AD23C, passes all 4 arguments to func_800AD398,
 *   calls func_800AD4E4(), and sets global state flag D_800CEA64 = 1.
 *   (Parallel to func_800AD0A4, which calls func_800AD3C8 instead of func_800AD398.)
 *
 * Arguments:
 *   arg0..arg3 (unsigned int) - forwarded directly to func_800AD398
 *
 * Return value:
 *   None (void)
 *
 * Global fields written:
 *   D_800CEA64 (u32) - set to 1 on completion
 */

extern unsigned int D_800CEA64;

extern void func_800AD588(void);
extern void func_800AD3D8(void);
extern void func_800AD430(void);
extern void func_800ACE68(void);
extern void func_800ACF08(int arg0);
extern void func_800AD23C(void);
extern void func_800AD398(unsigned int arg0, unsigned int arg1, unsigned int arg2, unsigned int arg3);
extern void func_800AD4E4(void);

void func_800AD13C(unsigned int arg0, unsigned int arg1, unsigned int arg2, unsigned int arg3)
{
    func_800AD588();
    func_800AD3D8();
    func_800AD430();
    func_800ACE68();
    func_800ACF08(0);
    func_800AD23C();
    func_800AD398(arg0, arg1, arg2, arg3);
    func_800AD4E4();
    D_800CEA64 = 1U;
}
