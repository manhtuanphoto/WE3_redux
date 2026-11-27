/*
 * func_800AD3C8
 *
 * Purpose:
 *   PS1 BIOS B0 vector trampoline: B(15h) - OutdatedPadInitAndStart(type, button_dest, unused, unused).
 *   Loads $t2 = 0xB0, $t1 = 0x15 and tail-jumps to 0x000000B0, forwarding
 *   caller arguments and $ra.
 *
 * Arguments:
 *   type (int): Pad init type ($a0)
 *   button_dest (void *): Destination pointer for pad state ($a1)
 *   arg2 (int): Unused / BIOS-specific ($a2)
 *   arg3 (int): Unused / BIOS-specific ($a3)
 *
 * Return value:
 *   int: BIOS status returned in $v0
 */

int func_800AD3C8(int type, void *button_dest, int arg2, int arg3)
{
    /* BIOS call: $t2 = 0xB0, $t1 = 0x15 (OutdatedPadInitAndStart), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x15 and tail-jump to 0x000000B0 without inline asm; exact caller argument count not visible in stub. */
    (void)type;
    (void)button_dest;
    (void)arg2;
    (void)arg3;
    return 0;
}
