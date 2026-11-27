/*
 * func_800C6068
 *
 * Purpose:
 *   PS1 BIOS B0 vector trampoline: B(5Ch) - get_card_status(slot) / _card_status(slot).
 *   Loads $t2 = 0xB0, $t1 = 0x5C and tail-jumps to 0x000000B0 to query the
 *   status of the most recent Memory Card command on `slot`.
 *
 * Arguments:
 *   slot (int): Memory Card slot/channel index ($a0; e.g. port >> 4)
 *
 * Return value:
 *   (int) Card status bitmask/code in $v0 (0x01 = ready, 0x02/0x04/0x08 = busy,
 *   0x11/0x21 = error/timeout).
 */

int func_800C6068(int slot)
{
    /* BIOS call: $t2 = 0xB0, $t1 = 0x5C (get_card_status), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x5C and tail-jump to 0x000000B0 without inline asm. */
    (void)slot;
    return 0;
}
