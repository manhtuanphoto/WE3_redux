/*
 * func_800B6D78
 *
 * Purpose:
 *   Initializes the CD-ROM subsystem (PsyQ libcd-style init). Attempts func_800B6EC4(1)
 *   up to 5 times; on success, registers three event/callback handlers (D_800B6DFC,
 *   D_800B6E24, D_800B6E4C) that signal BIOS event 0xF0000003, calls func_800B9524(0),
 *   and returns 1. On failure after 5 attempts, logs/reports D_8001515C via func_800AD698
 *   and returns 0.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   1 on initialization success, 0 on failure.
 *
 * Global fields referenced:
 *   D_8001515C (error message string)
 */

extern unsigned char D_8001515C[];

extern signed int func_800B6EC4(unsigned int param);
extern void func_800B700C(void (*cb)(void));
extern void func_800B7020(void (*cb)(void));
extern void func_800B9510(void (*cb)(void));
extern void func_800B9524(unsigned int param);
extern void func_800AD698(unsigned char *msg);
extern void func_800B6E78(unsigned int event_id, unsigned int flags);

static void D_800B6DFC(void)
{
    func_800B6E78(0xF0000003U, 0x20);
}

static void D_800B6E24(void)
{
    func_800B6E78(0xF0000003U, 0x40);
}

static void D_800B6E4C(void)
{
    func_800B6E78(0xF0000003U, 0x40);
}

unsigned int func_800B6D78(void)
{
    signed int retries = 4;

    do {
        if (func_800B6EC4(1) == 1) {
            func_800B700C(D_800B6DFC);
            func_800B7020(D_800B6E24);
            func_800B9510(D_800B6E4C);
            func_800B9524(0);
            return 1;
        }
        retries--;
    } while (retries != -1);

    func_800AD698(D_8001515C);
    return 0;
}
