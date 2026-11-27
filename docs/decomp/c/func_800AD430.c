/*
 * func_800AD430
 *
 * Purpose:
 *   Low-level PS1 BIOS / callback table initialization routine.
 *   Saves the caller's return address in D_800E7018, calls func_800AD3D8(),
 *   invokes PS1 BIOS B0 function 0x57 (via 0xB0 with $t1 = 0x57), reads the
 *   pointer at +0x16C of the returned BIOS struct, records (+0x884) and
 *   (+0x894) into jtbl_800E7020 and jtbl_800E7024, zeroes 11 32-bit words
 *   starting at +0x594, calls func_800AD4A8(), and restores $ra from D_800E7018.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read / written:
 *   D_800E7018 (void *): saved return address
 *   jtbl_800E7020 (void *): set to base_ptr + 0x884
 *   jtbl_800E7024 (void *): set to base_ptr + 0x894
 */

#define W(p, o) (*(unsigned int *)((unsigned char *)(p) + (o)))

extern void *D_800E7018;
extern void *jtbl_800E7020;
extern void *jtbl_800E7024;

extern void func_800AD3D8(void);
extern void func_800AD4A8(void);
extern unsigned char *bios_b0_57(void); /* PS1 BIOS call: $t2 = 0xB0, $t1 = 0x57 */

void func_800AD430(void)
{
    unsigned char *sys_tbl;
    unsigned char *base_ptr;
    int i;

    /* UNCLEAR: $ra is preserved in global D_800E7018 rather than on the stack */
    func_800AD3D8();

    /* UNCLEAR: exact BIOS B0:57 table layout; dereferences +0x16C and patches +0x594..+0x5BC */
    sys_tbl = bios_b0_57();
    base_ptr = (unsigned char *)W(sys_tbl, 0x16C);

    jtbl_800E7020 = base_ptr + 0x884;
    jtbl_800E7024 = base_ptr + 0x894;

    for (i = 0; i < 11; i++) {
        W(base_ptr + i * 4, 0x594) = 0;
    }

    func_800AD4A8();
}
