/*
 * func_800AD408
 *
 * Purpose:
 *   Indirect tail-call trampoline that jumps (`jr $t1`) to the function
 *   pointer stored in jtbl_800E7020 (initialized by func_800AD430 from BIOS
 *   B0:57 table +0x16C -> +0x884).
 *
 * Arguments:
 *   Forwarded unchanged to *jtbl_800E7020.
 *
 * Return value:
 *   Forwarded unchanged from *jtbl_800E7020.
 *
 * Global fields read:
 *   jtbl_800E7020 (void *): target function pointer
 */

extern void (*jtbl_800E7020)(void);

void func_800AD408(void)
{
    /* UNCLEAR: exact argument signature and return type of the target callback at jtbl_800E7020 */
    jtbl_800E7020();
}
