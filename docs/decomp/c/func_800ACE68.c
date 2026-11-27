/*
 * func_800ACE68
 *
 * Purpose:
 *   PS1 BIOS system call SYS(02h) - ExitCriticalSection().
 *   Executes `syscall 0` with $a0 = 2 to re-enable interrupts after a critical
 *   section.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 */

void func_800ACE68(void)
{
    /* PS1 BIOS syscall(2): ExitCriticalSection ($a0 = 2, syscall 0) */
    /* UNCLEAR: standard C cannot emit `syscall 0` directly without inline asm/intrinsic. */
}
