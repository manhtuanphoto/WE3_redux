/*
 * func_800BAE48
 *
 * Purpose:
 *   PS1 BIOS A0 vector trampoline: A(13h) - setjmp(buf) / SaveState(buf).
 *   Loads $t2 = 0xA0, $t1 = 0x13 and tail-jumps to 0x000000A0, saving
 *   callee-saved registers ($ra, $sp, $fp, $s0..$s7, $gp) into the 48-byte
 *   buffer at $a0.
 *
 * Arguments:
 *   buf (void *): 48-byte (0x30) jmp_buf register save buffer ($a0)
 *
 * Return value:
 *   (int) 0 when called directly; non-zero when resumed via exception exit / longjmp.
 */

int func_800BAE48(void *buf)
{
    /* BIOS call: $t2 = 0xA0, $t1 = 0x13 (setjmp / SaveState), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x13 and tail-jump to 0x000000A0 without inline asm. */
    (void)buf;
    return 0;
}
