/*
 * func_800B72A4
 *
 * Purpose:
 *   Blocking CD-ROM command wrapper with up to 4 retry attempts (analogous to
 *   PsyQ CdControlB). Optionally issues command 1 (CdlNop) if D_800D395C & 0x10
 *   and cmd != 1, optionally issues command 2 (CdlSetloc) with param if
 *   param != 0 and D_800D38C4[cmd] != 0, then issues `cmd` via func_800B80BC.
 *   On success, waits/checks CD completion via func_800B7B74(0, result) == 2.
 *
 * Arguments:
 *   cmd    (unsigned char): CD command byte
 *   param  (void *): command parameter buffer (or NULL)
 *   result (void *): CD result buffer (or NULL)
 *
 * Return value:
 *   1 if command succeeded and func_800B7B74(0, result) == 2;
 *   0 on command failure after 4 retries or non-2 status.
 *
 * Global fields read / written:
 *   D_800D38C4 (u32[]): per-command flag table (non-zero triggers seek/setloc cmd 2)
 *   D_800D394C (u32): saved/temporarily cleared CD callback or flag
 *   D_800D395C (u8): CD status/mode flags (bit 4 = 0x10)
 */

extern unsigned int D_800D38C4[];
extern unsigned int D_800D394C;
extern unsigned char D_800D395C;

extern signed int func_800B80BC(unsigned int cmd, void *param, void *result, unsigned int arg3);
extern signed int func_800B7B74(signed int mode, void *result);

signed int func_800B72A4(unsigned int cmd, void *param, void *result)
{
    unsigned char cmdByte = (unsigned char)(cmd & 0xFFU);
    unsigned int savedFlag = D_800D394C;
    signed int retries = 3;
    signed int status = 0;

    do {
        D_800D394C = 0;
        if (cmdByte != 1 && (D_800D395C & 0x10U) != 0) {
            func_800B80BC(1, 0, 0, 0);
        }

        if (param != 0 && D_800D38C4[cmdByte] != 0) {
            if (func_800B80BC(2, param, result, 0) != 0) {
                retries--;
                continue;
            }
        }

        D_800D394C = savedFlag;
        if (func_800B80BC(cmdByte, param, result, 0) == 0) {
            status = 0;
            break;
        }

        retries--;
    } while (retries != -1);

    if (retries == -1) {
        D_800D394C = savedFlag;
        status = -1;
    }

    if (status == 0) {
        return (func_800B7B74(0, result) == 2) ? 1 : 0;
    }

    return 0;
}
