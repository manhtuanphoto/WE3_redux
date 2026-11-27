/*
 * func_800A9394
 *
 * Purpose:
 *   Executes a 3-step CD-ROM control sequence via func_800B7170 (commands 0x0E,
 *   0x0D, and 0x06), retrying each up to 257 (0x101) times using D_800E6BAA as
 *   the retry counter. Optionally copies 3 bytes from D_800E6AFD..D_800E6AFF
 *   into D_800E6AE0..D_800E6AE2 before command 0x06 when `param != 0`.
 *
 * Arguments:
 *   param (s16): if non-zero, updates D_800E6AE0[0..2] from D_800E6AFD..D_800E6AFF
 *
 * Return value:
 *   0 if all commands succeed within retry limits; -1 if any command times out.
 *
 * Global fields read/written:
 *   D_800E6BD9 (u8)  cleared to 0
 *   D_800E6BAA (u16) retry counter (0..0x101)
 *   D_800E6AE8 (u8)  param byte 0 for command 0x0D
 *   D_800E6BD4 (u8)  param byte 1 for command 0x0D
 *   D_800E6AFD / D_800E6AFE / D_800E6AFF (u8) source bytes for command 0x06
 *   D_800E6AE0 / D_800E6AE1 / D_800E6AE2 (u8[]) parameter buffer for command 0x06
 *   D_800E6B4C (u8)  cleared to 0 on command 0x06 timeout
 */

extern unsigned char D_800E6BD9;
extern unsigned short D_800E6BAA;
extern unsigned char D_800E6AE8;
extern unsigned char D_800E6BD4;
extern unsigned char D_800E6AFD;
extern unsigned char D_800E6AFE;
extern unsigned char D_800E6AFF;
extern unsigned char D_800E6AE0[];
extern unsigned char D_800E6AE1;
extern unsigned char D_800E6AE2;
extern unsigned char D_800E6B4C;

extern int func_800B7170(unsigned int cmd, unsigned char *buf);

signed int func_800A9394(signed short param)
{
    unsigned char buf[16];
    signed int status = 0;

    D_800E6BD9 = 0;
    D_800E6BAA = 0;
    buf[0] = 0xCB;
    D_800E6BAA = 0;

    while (func_800B7170(0xEU, buf) == 0) {
        D_800E6BAA++;
        if ((signed short)D_800E6BAA >= 0x101) {
            status = -1;
            break;
        }
    }

    D_800E6BAA = 0;
    buf[0] = D_800E6AE8;
    buf[1] = D_800E6BD4;

    while (func_800B7170(0xDU, buf) == 0) {
        D_800E6BAA++;
        if ((signed short)D_800E6BAA >= 0x101) {
            status = -1;
            break;
        }
    }

    if (param != 0) {
        D_800E6AE0[0] = D_800E6AFD;
        D_800E6AE1 = D_800E6AFE;
        D_800E6AE2 = D_800E6AFF;
    }

    while (func_800B7170(6U, D_800E6AE0) == 0) {
        D_800E6BAA++;
        if ((signed short)D_800E6BAA >= 0x101) {
            status = -1;
            D_800E6B4C = 0;
            return status;
        }
    }

    return status;
}
