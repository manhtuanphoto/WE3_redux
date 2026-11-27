/*
 * func_800C59B8
 *
 * Purpose:
 *   Initializes the PS1 Memory Card / SIO subsystem inside a critical section:
 *   calls func_800ACF08(0), enters critical section (func_800AD3D8), checks
 *   func_800AD094() (forcing arg0 to 0 if it returns 0), calls initialization
 *   helpers func_800C5A88(arg0), func_800C5C54(), func_800C5B50(), and
 *   func_800C5BE4(), and flushes cache via func_800ACE68().
 *
 * Arguments:
 *   arg0 (int): mode/shared flag passed to func_800C5A88 (zeroed if func_800AD094() == 0)
 *
 * Return value:
 *   None (void)
 */

extern void func_800ACF08(int disable);
extern void func_800AD3D8(void);
extern int  func_800AD094(void);
extern void func_800C5A88(int arg0);
extern void func_800C5C54(void);
extern void func_800C5B50(void);
extern void func_800C5BE4(void);
extern void func_800ACE68(void);

void func_800C59B8(int arg0)
{
    int mode = arg0;

    func_800ACF08(0);
    func_800AD3D8();

    if (func_800AD094() == 0) {
        mode = 0;
    }

    func_800C5A88(mode);
    func_800C5C54();
    func_800C5B50();
    func_800C5BE4();
    func_800ACE68();
}
