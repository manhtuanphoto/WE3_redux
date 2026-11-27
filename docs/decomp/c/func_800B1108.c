/*
 * func_800B1108
 *
 * Purpose:
 *   PS1 BIOS A0 vector trampoline: A(49h) - GPU_cw(gp0cmd).
 *   Loads $t2 = 0xA0, $t1 = 0x49 and tail-jumps to 0x000000A0, forwarding
 *   $a0 (32-bit GP0 command word) to send a single command/data word to the GPU.
 *
 * Arguments:
 *   gp0cmd (unsigned int): 32-bit GP0 command word ($a0)
 *
 * Return value:
 *   None (void)
 */

void func_800B1108(unsigned int gp0cmd)
{
    /* BIOS call: $t2 = 0xA0, $t1 = 0x49 (GPU_cw), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x49 and tail-jump to 0x000000A0 without inline asm. */
    (void)gp0cmd;
}
