/*
 * func_800AD588
 *
 * Purpose:
 *   Enters a critical section (func_800AD3D8), queries a PS1 BIOS B0 table
 *   pointer via syscall vector 0xB0 with $t1 = 0x57, dereferences +0x16C and
 *   adds 0x62C, zeroes 9 consecutive 32-bit words (36 bytes), then exits the
 *   critical section (func_800AD4A8) and flushes the instruction cache
 *   (func_800ACE68). Saves/restores return address in D_800E7038 instead of
 *   the stack.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800E7038 (unsigned int): temporary storage for saved $ra
 *
 * Notes:
 *   UNCLEAR: Exact kernel/BIOS structure cleared at `*(B0(0x57) + 0x16C) + 0x62C`
 *   (9 words = 36 bytes). Also note `addi $v1, $v0, 0x62C` is marked as a
 *   handwritten instruction in the assembly.
 */

extern unsigned int D_800E7038;

extern void func_800AD3D8(void);
extern void func_800AD4A8(void);
extern void func_800ACE68(void);

/* UNCLEAR: PS1 BIOS B0 syscall invoked directly via jalr 0xB0 with $t1 = 0x57 */
static unsigned char *bios_b0_57(void)
{
    /* Performs jalr 0xB0 with $t1 = 0x57 and returns pointer in $v0 */
    return ((unsigned char *(*)(void))0x800000B0)(); /* UNCLEAR: requires $t1 = 0x57 in register */
}

void func_800AD588(void)
{
    unsigned char *tbl;
    unsigned int *ptr;
    int i;

    /* Assembly saves $ra into D_800E7038 and restores it before jr $ra */
    func_800AD3D8();

    tbl = bios_b0_57();
    ptr = (unsigned int *)(*(unsigned char **)(tbl + 0x16C) + 0x62C);

    for (i = 9; i != 0; i--) {
        *ptr++ = 0;
    }

    func_800AD4A8();
    func_800ACE68();
}
