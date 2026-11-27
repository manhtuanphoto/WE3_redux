/*
 * func_800AD4A8
 *
 * Purpose:
 *   PS1 BIOS A-table syscall stub for A(0x44) (jumps to 0x000000A0 with
 *   $t1 = 0x44, returning directly to caller via $ra).
 *
 *   Note: The assembly file also contains an unreachable adjacent function
 *   at offset +0x0C (lines 5-12 of func_800AD4A8.s) following the unconditional
 *   `jr $t2`: it saves $ra, loads function pointer D_800E702C, and calls it
 *   via `jalr $t1` (with `lw $ra, 0x14($sp)` in the jalr delay slot before
 *   $ra is overwritten by jalr).
 *
 * Arguments:
 *   Forwarded in $a0..$a3 to BIOS A(0x44)
 *
 * Return value:
 *   Forwarded from BIOS A(0x44) in $v0
 *
 * Global fields read (in unreachable trailing block):
 *   D_800E702C (void (*)(void))
 */

void func_800AD4A8(void)
{
    /* UNCLEAR: Direct PS1 BIOS A-table tail-jump (`t1 = 0x44; jr 0x000000A0`,
     * standard PS1 BIOS A(44h) CdInit stub). Cannot be expressed in portable C. */
    ((void (*)(void))0x000000A0)();

    /* Unreachable trailing code in func_800AD4A8.s (+0x0C..+0x28, likely an
     * unsplit adjacent function):
     *   extern void (*D_800E702C)(void);
     *   D_800E702C();
     */
}
