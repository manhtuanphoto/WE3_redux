/*
 * func_800AD3D8
 *
 * Purpose:
 *   PS1 BIOS system call SYS(01h) - EnterCriticalSection().
 *   Executes `syscall 0` with $a0 = 1 to disable interrupts and enter a
 *   critical section.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   int: Previous interrupt enable state returned by BIOS in $v0 (1 if enabled, 0 if already disabled)
 */

int func_800AD3D8(void)
{
    /* PS1 BIOS syscall(1): EnterCriticalSection ($a0 = 1, syscall 0) */
    /* UNCLEAR: standard C cannot emit `syscall 0` directly without inline asm/intrinsic; returns BIOS $v0. */
    return 0;
}
