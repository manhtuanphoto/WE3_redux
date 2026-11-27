/*
 * func_800B6EC4
 *
 * Purpose:
 *   PsyQ CD-ROM subsystem reset/initialization dispatcher (CdReset / CdInit
 *   style):
 *     - mode == 2: calls func_800B8714() and returns 1.
 *     - mode != 2: calls func_800B8760() (low-level CD init); returns 0 on
 *       failure (non-zero return). If mode == 1, also calls func_800B8624()
 *       (CdInitVol) and returns 0 on failure, else returns 1.
 *
 * Arguments:
 *   mode (int): initialization mode (2 = warm/reset path via func_800B8714,
 *               1 = full CD + SPU/CD volume init, other = CD init only)
 *
 * Return value:
 *   int: 1 on success, 0 on failure.
 */

extern void func_800B8714(void);
extern int func_800B8760(void);
extern int func_800B8624(void);

int func_800B6EC4(int mode)
{
    if (mode == 2) {
        func_800B8714();
        return 1;
    }

    if (func_800B8760() != 0) {
        return 0;
    }

    if (mode == 1) {
        if (func_800B8624() != 0) {
            return 0;
        }
    }

    return 1;
}
