/*
 * func_800A9128
 *
 * Purpose:
 *   Blocking CD-ROM sector read with automatic retry. Converts `sector` to a
 *   CdlLOC via func_800B7494, seeks with CdlSetloc (cmd 2), verifies sync status
 *   is not CdlDiskError (5), reads `count` sectors into `buf` via func_800B9348
 *   (mode 0x80), and waits for completion with func_800B9448. On error or
 *   timeout (-1), issues two CdlNop (cmd 1) commands and retries.
 *
 * Arguments:
 *   sector (int)  - starting logical CD sector number
 *   count  (int)  - number of sectors to read
 *   buf    (u8*)  - destination buffer in RAM
 *
 * Return value:
 *   int - always returns 0 once the read succeeds
 */

extern void func_800B7494(int sector, unsigned char *locOut);
extern int  func_800B72A4(unsigned int cmd, unsigned char *param, unsigned char *result);
extern int  func_800B6FCC(int mode, unsigned char *result);
extern int  func_800B9348(int count, unsigned char *buf, unsigned int mode);
extern int  func_800B9448(int mode, unsigned char *result);

int func_800A9128(int sector, int count, unsigned char *buf)
{
    unsigned char loc[16]; /* sp+0x10..0x1F (4-byte CdlLOC + padding) */

    while (1) {
        func_800B7494(sector, loc);
        func_800B72A4(2, loc, 0); /* CdlSetloc */

        if (func_800B6FCC(0, 0) != 5) {
            func_800B9348(count, buf, 0x80);
            if (func_800B9448(0, 0) != -1) {
                return 0;
            }
        }

        func_800B72A4(1, 0, 0); /* CdlNop */
        func_800B72A4(1, 0, 0); /* CdlNop */
    }
}
