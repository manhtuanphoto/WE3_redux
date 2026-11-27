/*
 * func_800A95C0
 *
 * Purpose:
 *   Issues CD-ROM control command 6 with parameter buffer D_800E6AE0 via
 *   func_800B7170(6, D_800E6AE0) (PsyQ CdControl wrapper).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   (int) 1 on success, 0 on failure (forwarded from func_800B7170)
 *
 * Global fields referenced:
 *   D_800E6AE0 (u8[]): CD-ROM command parameter buffer (e.g., CdlLOC)
 */

extern unsigned char D_800E6AE0[];

extern int func_800B7170(unsigned int cmd, unsigned char *param);

int func_800A95C0(void)
{
    return func_800B7170(6U, D_800E6AE0);
}
