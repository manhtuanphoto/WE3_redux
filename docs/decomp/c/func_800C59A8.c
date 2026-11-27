/*
 * func_800C59A8
 *
 * Purpose:
 *   PS1 BIOS B0 vector trampoline: B(50h) - allow_new_card() / _new_card().
 *   Loads $t2 = 0xB0, $t1 = 0x50 and tail-jumps to 0x000000B0 to clear the
 *   BIOS card-change flag so the next Memory Card read/write is allowed.
 *
 * Arguments:
 *   None (void)
 *   (Note: callers such as func_800C5D08 pass `port` in $a0, which is ignored
 *   by BIOS B(50h) and left in $a0 if not clobbered.)
 *
 * Return value:
 *   None (void)
 */

void func_800C59A8(void)
{
    /* BIOS call: $t2 = 0xB0, $t1 = 0x50 (allow_new_card / _new_card), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x50 and tail-jump to 0x000000B0 without inline asm. */
}
