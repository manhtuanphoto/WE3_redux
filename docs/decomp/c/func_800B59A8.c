/*
 * func_800B59A8
 *
 * Purpose:
 *   Sets the 3D lighting/projection mode in D_8010C298 (equivalent to PsyQ
 *   GsSetLightMode). Accepts modes 0..3; any other value logs an error/debug
 *   message at D_800150AC via func_800AD698.
 *
 * Arguments:
 *   mode (int) - lighting mode (valid values: 0, 1, 2, 3)
 *
 * Return value:
 *   None (void)
 *
 * Global fields referenced:
 *   D_8010C298 (s32, W): active lighting mode
 *   D_800150AC (const char *, R): error/warning message string
 */

extern signed int D_8010C298;
extern const char D_800150AC[];

extern void func_800AD698(const char *msg);

void func_800B59A8(int mode)
{
    switch (mode) {
    case 0:
        D_8010C298 = 0;
        break;
    case 1:
    case 2:
    case 3:
        D_8010C298 = mode;
        break;
    default:
        func_800AD698(D_800150AC);
        break;
    }
}
