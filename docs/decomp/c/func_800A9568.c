/*
 * func_800A9568
 *
 * Purpose:
 *   Configures CD-ROM XA playback mode and filter via func_800B7170 (CdControl).
 *   Clears D_800E6BD9 to 0, issues CD command 0x0E (CdlSetmode) with mode byte
 *   0xCB, and then issues CD command 0x0D (CdlSetfilter) with file = D_800E6AE8
 *   and channel = D_800E6BD4.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   int: return value from the second func_800B7170(0x0D, ...) call
 *
 * Global fields read/written:
 *   D_800E6BD9 (u8): cleared to 0
 *   D_800E6AE8 (u8): CD filter file number
 *   D_800E6BD4 (u8): CD filter channel number
 */

extern unsigned char D_800E6BD9;
extern unsigned char D_800E6AE8;
extern unsigned char D_800E6BD4;

extern int func_800B7170(unsigned int cmd, unsigned char *param);

int func_800A9568(void)
{
    unsigned char param[16];

    D_800E6BD9 = 0;
    param[0] = 0xCB;
    func_800B7170(0x0Eu, param); /* CdlSetmode (0xCB) */

    param[0] = D_800E6AE8;
    param[1] = D_800E6BD4;
    return func_800B7170(0x0Du, param); /* CdlSetfilter (file, channel) */
}
