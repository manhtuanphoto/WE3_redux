/*
 * func_800C5C88
 *
 * Purpose:
 *   Patches PS1 kernel C0 vector 0x06 (at C0_table[6] + 0x70) inside a critical
 *   section. Saves $ra to D_800E7338, enters critical section via func_800AD3D8,
 *   queries the BIOS C0 table via B0:0x56 (GetC0Table), copies 3 words
 *   (D_800C5CF8..D_800C5D04, 12 bytes) into *(c0_table + 0x18) + 0x70, exits
 *   the critical section via func_800AD4A8, and flushes instruction cache via
 *   func_800ACE68.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800E7338 (u32): static save slot for return address $ra (non-reentrant)
 *   D_800C5CF8..D_800C5D04 (u32[3]): 12-byte code patch copied to C0[6] + 0x70
 */

extern unsigned int D_800E7338;
extern unsigned int D_800C5CF8[];
extern unsigned int D_800C5D04[];

extern void func_800AD3D8(void);
extern void func_800AD4A8(void);
extern void func_800ACE68(void);

/* BIOS B0:0x56 (GetC0Table) returns base address of kernel C0 jump table */
static unsigned char **bios_GetC0Table(void)
{
    register unsigned int t1 __asm__("$9") = 0x56;
    register unsigned int t2 __asm__("$10") = 0xB0;
    unsigned char **ret;
    __asm__ volatile("jalr %1" : "=v0"(ret) : "r"(t2), "r"(t1));
    return ret;
}

void func_800C5C88(void)
{
    unsigned char **c0_table;
    unsigned int *dst;
    unsigned int *src;

    func_800AD3D8();
    c0_table = bios_GetC0Table();
    dst = (unsigned int *)(c0_table[0x18 / 4] + 0x70);

    for (src = D_800C5CF8; src != D_800C5D04; src++, dst++) {
        *dst = *src;
    }

    func_800AD4A8();
    func_800ACE68();
}
