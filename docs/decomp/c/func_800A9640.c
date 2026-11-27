/*
 * func_800A9640
 *
 * Purpose:
 *   Blocking CD-ROM pause helper. Loops polling func_800B6FCC(1, 0) (CdSync):
 *   when status != 2, pumps CD control via func_800B7034(1, 0, 0) (CdlNop);
 *   when status == 2, issues command 9 (CdlPause) via func_800B72A4(9, 0, 0)
 *   and exits once func_800B6FCC(1, 0) again returns 2.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 */

extern int func_800B6FCC(int mode, unsigned char *result);
extern int func_800B72A4(unsigned int cmd, unsigned char *param, unsigned char *result);
extern int func_800B7034(unsigned int cmd, unsigned char *param, unsigned char *result);

void func_800A9640(void)
{
    for (;;) {
        int status = func_800B6FCC(1, 0);

        if (status == 2) {
            func_800B72A4(9U, 0, 0);
            if (func_800B6FCC(1, 0) == status) {
                break;
            }
        } else {
            func_800B7034(1U, 0, 0);
        }
    }
}
