/*
 * func_800B7170
 *
 * Purpose:
 *   CD-ROM control command wrapper (PsyQ CdControl-style routine). Retries up
 *   to 4 times (loop counter 3 down to 0). Before sending the command, optionally
 *   issues a status/nop command (1) if cmd != 1 and (D_800D395C & 0x10) != 0,
 *   and optionally issues Setloc (2) when param != NULL and D_800D38C4[cmd] != 0.
 *
 * Arguments:
 *   cmd (unsigned char / unsigned int): CD-ROM command code (masked to 0xFF)
 *   param (unsigned char *): optional command parameter buffer (e.g., CdlLOC)
 *
 * Return value:
 *   1 if func_800B80BC(cmd & 0xFF, param, 0, 1) succeeds (returns 0);
 *   0 if all 4 attempts fail.
 *
 * Global fields read:
 *   D_800D38C4 (unsigned int[]): per-command table indicating whether param requires cmd 2 first
 *   D_800D394C (unsigned int): saved/restored CD callback or state word
 *   D_800D395C (unsigned char): CD status/flag byte (bit 4 = 0x10 triggers pre-cmd 1)
 *
 * Global fields written:
 *   D_800D394C (unsigned int): temporarily cleared to 0 during pre-commands, then restored
 */

extern unsigned int D_800D38C4[];
extern unsigned int D_800D394C;
extern unsigned char D_800D395C;

extern int func_800B80BC(unsigned int cmd, unsigned char *param, unsigned int a2, unsigned int a3);

int func_800B7170(unsigned int cmd, unsigned char *param)
{
    unsigned int cmd_byte = cmd & 0xFFu;
    unsigned int saved_d394c = D_800D394C;
    int retries = 3;

    do {
        D_800D394C = 0;

        if (cmd_byte != 1u && (D_800D395C & 0x10u) != 0) {
            func_800B80BC(1u, 0, 0u, 0u);
        }

        if (param != 0 && D_800D38C4[cmd_byte] != 0) {
            if (func_800B80BC(2u, param, 0u, 0u) != 0) {
                continue;
            }
        }

        D_800D394C = saved_d394c;
        if (func_800B80BC(cmd_byte, param, 0u, 1u) == 0) {
            return 1;
        }
    } while (--retries != -1);

    D_800D394C = saved_d394c;
    return 0;
}
