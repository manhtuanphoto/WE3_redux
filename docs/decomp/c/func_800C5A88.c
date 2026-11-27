/*
 * func_800C5A88
 *
 * Purpose:
 *   PS1 BIOS B0 vector trampoline: B(4Ah) - InitCARD(pad_enable).
 *   Loads $t2 = 0xB0, $t1 = 0x4A and tail-jumps to 0x000000B0, forwarding
 *   $a0 to initialize the low-level Memory Card BIOS driver.
 *
 * Arguments:
 *   pad_enable (int): Shared controller/pad enable flag ($a0; 0 = card only, 1 = pad + card)
 *
 * Return value:
 *   None (void)
 */

void func_800C5A88(int pad_enable)
{
    /* BIOS call: $t2 = 0xB0, $t1 = 0x4A (InitCARD), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x4A and tail-jump to 0x000000B0 without inline asm. */
    (void)pad_enable;
}
