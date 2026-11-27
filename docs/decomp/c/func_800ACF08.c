/*
 * func_800ACF08
 *
 * Purpose:
 *   PS1 BIOS B0 vector trampoline: B(5Bh) - ChangeClearPad(int_flag).
 *   Loads $t2 = 0xB0, $t1 = 0x5B and tail-jumps to 0x000000B0, forwarding $a0
 *   and $ra.
 *
 * Arguments:
 *   int_flag (int): Pad interrupt clear mode ($a0)
 *
 * Return value:
 *   None (void)
 */

void func_800ACF08(int int_flag)
{
    /* BIOS call: $t2 = 0xB0, $t1 = 0x5B (ChangeClearPad), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x5B and tail-jump to 0x000000B0 without inline asm. */
    (void)int_flag;
}
