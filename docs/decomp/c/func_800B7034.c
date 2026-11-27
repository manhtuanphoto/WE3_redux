/*
 * func_800B7034
 *
 * Purpose:
 *   Non-blocking CD-ROM command wrapper with up to 4 retry attempts (analogous
 *   to PsyQ CdControl). Optionally issues command 1 (CdlNop) if D_800D395C & 0x10
 *   and cmd != 1, optionally issues command 2 (CdlSetloc) with param if
 *   param != 0 and D_800D38C4[cmd] != 0, then issues `cmd` via func_800B80BC.
 *
 * Arguments:
 *   cmd    (unsigned char): CD command byte
 *   param  (void *): command parameter buffer (or NULL)
 *   result (void *): CD result buffer (or NULL)
 *
 * Return value:
 *   1 if the command was issued successfully; 0 after 4 failed retries.
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

signed int func_800B7034(unsigned int cmd, void *param, void *result)
{
    unsigned char cmdByte = (unsigned char)(cmd & 0xFFU);
    unsigned int savedFlag = D_800D394C;
    signed int retries = 3;

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
            return 1;
        }

        retries--;
    } while (retries != -1);

    D_800D394C = savedFlag;
    return 0;
}
