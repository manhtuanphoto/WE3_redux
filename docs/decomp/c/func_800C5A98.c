/*
 * func_800C5A98
 *
 * Purpose:
 *   PS1 BIOS B0 vector trampoline: B(4Bh) - StartCARD().
 *   Loads $t2 = 0xB0, $t1 = 0x4B and tail-jumps to 0x000000B0 to enable/start
 *   Memory Card BIOS interrupt handling after InitCARD.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 */

void func_800C5A98(void)
{
    /* BIOS call: $t2 = 0xB0, $t1 = 0x4B (StartCARD), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x4B and tail-jump to 0x000000B0 without inline asm. */
}
