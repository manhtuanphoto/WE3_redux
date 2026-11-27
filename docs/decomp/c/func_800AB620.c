/*
 * func_800AB620
 *
 * Purpose:
 *   Top-level audio/sound-command dispatcher. Only executes when sound driver
 *   state D_800E6B88 == 2. Sets busy/guard flag D_800E6AEA = 1, routes `cmd`
 *   by range:
 *     - cmd < 0x100:          func_800AB698(cmd)  (control/menu-flow command)
 *     - 0x100 <= cmd < 0x500: func_800ABD5C(cmd)  (BGM/track command)
 *     - 0x500 <= cmd <= 0xEFFF: func_800ABE7C(cmd) (SFX/cue command)
 *     - cmd > 0xEFFF:         ignored
 *   Then clears D_800E6AEA = 0.
 *
 * Arguments:
 *   cmd (signed int): audio/sound command ID
 *
 * Return value:
 *   None (void)
 *
 * Global fields read / written:
 *   D_800E6B88 (unsigned char, gp-relative): audio subsystem state (must be 2)
 *   D_800E6AEA (unsigned char, gp-relative): dispatch busy flag (1 while active, 0 on exit)
 */

extern unsigned char D_800E6B88;
extern unsigned char D_800E6AEA;

extern void func_800AB698(unsigned int cmd);
extern void func_800ABD5C(int track_id);
extern void func_800ABE7C(int cue_id);

void func_800AB620(signed int cmd)
{
    if (D_800E6B88 != 2) {
        return;
    }

    D_800E6AEA = 1;

    if (cmd < 0x100) {
        func_800AB698((unsigned int)cmd);
    } else if (cmd < 0x500) {
        func_800ABD5C(cmd);
    } else if (cmd <= 0xEFFF) {
        func_800ABE7C(cmd);
    }

    D_800E6AEA = 0;
}
