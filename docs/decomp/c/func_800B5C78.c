/*
 * func_800B5C78
 *
 * Purpose:
 *   Patches the PS1 BIOS C0 exception handler if it matches the expected 6-word
 *   instruction sequence (D_800B5D24..D_800B5D3C). Enters critical section (func_800AD3D8),
 *   invokes PS1 BIOS syscall B0:56 (GetC0Table), locates *(c0_table + 0x18) + 0x28,
 *   compares the 6 words against D_800B5D24, and on exact match overwrites them with
 *   the 6-word sequence at D_800B5D3C (which saves COP0 Cause $13 and EPC $14).
 *   Then exits critical section (func_800AD4A8) and flushes the instruction cache (func_800ACE68).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields referenced:
 *   D_800E71B8 (unsigned int): saves/restores $ra without a stack frame
 *   D_800B5D24 (const unsigned int[6]): expected BIOS handler prologue words:
 *     sw $at, 4($k0); sw $v0, 8($k0); sw $v1, 0xC($k0); sw $ra, 0x7C($k0); mfc0 $v1, $14; nop
 *   D_800B5D3C (const unsigned int[6]): replacement handler words:
 *     sw $at, 4($k0); sw $v0, 8($k0); mfc0 $v0, $13; sw $v1, 0xC($k0); mfc0 $v1, $14; sw $ra, 0x7C($k0)
 */

extern unsigned int D_800E71B8;
extern const unsigned int D_800B5D24[6];
extern const unsigned int D_800B5D3C[6];

extern void func_800AD3D8(void);
extern void func_800AD4A8(void);
extern void func_800ACE68(void);

/* Helper representing PS1 BIOS B0:56 (GetC0Table) */
static unsigned int *ps1_bios_get_c0_table(void)
{
    /* BIOS dispatch at 0x000000B0 with $t1 = 0x56 */
    typedef unsigned int *(*bios_b0_fn)(void);
    /* UNCLEAR: exact calling wrapper; in asm, $t1=0x56, $t2=0xB0, jalr $t2 */
    return ((bios_b0_fn)0x000000B0)();
}

void func_800B5C78(void)
{
    unsigned int *c0_table;
    unsigned int *patch_addr;
    int i;
    int match = 1;

    /* Note: asm stores $ra directly in global D_800E71B8 instead of stack */
    func_800AD3D8();

    c0_table = ps1_bios_get_c0_table();
    patch_addr = (unsigned int *)(c0_table[6] + 0x28);

    for (i = 0; i < 6; i++) {
        if (D_800B5D24[i] != patch_addr[i]) {
            match = 0;
            break;
        }
    }

    if (match) {
        for (i = 0; i < 6; i++) {
            patch_addr[i] = D_800B5D3C[i];
        }
    }

    func_800AD4A8();
    func_800ACE68();
}
