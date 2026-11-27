/*
 * func_800A961C
 *
 * Purpose:
 *   Issues CD-ROM command 9 (CdlPause) with a NULL parameter via func_800B7170
 *   and returns its status result.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   int: 1 on success, 0 on failure (forwarded from func_800B7170)
 */

extern int func_800B7170(unsigned int cmd, unsigned char *param);

int func_800A961C(void)
{
    return func_800B7170(9U, 0);
}
