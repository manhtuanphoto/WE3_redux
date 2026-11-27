/*
 * func_800C5BE4
 *
 * Purpose:
 *   Patches PS1 kernel B0 vector 0x5B (at B0_table[0x5B] + 0x9C8) inside a
 *   critical section. Saves $ra to D_800E7328, enters critical section via
 *   func_800AD3D8, queries the BIOS B0 table via B0:0x57 (GetB0Table), reads
 *   *(b0_table + 0x16C) + 0x9C8 (unused read of old word), copies words from
 *   D_800C5B3C up to func_800C5B50 (20 bytes / 5 words) into entry + 0x9C8,
 *   and exits the critical section via func_800AD4A8.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800E7328 (u32): static save slot for return address $ra (non-reentrant)
 *   D_800C5B3C..func_800C5B50 (u32[5]): code patch copied to B0[0x5B] + 0x9C8
 */

extern unsigned int D_800E7328;
extern unsigned int D_800C5B3C[];
extern unsigned int func_800C5B50[];

extern void func_800AD3D8(void);
extern void func_800AD4A8(void);

/* BIOS B0:0x57 (GetB0Table) returns base address of kernel B0 jump table */
static unsigned char **bios_GetB0Table(void)
{
    register unsigned int t1 __asm__("$9") = 0x57;
    register unsigned int t2 __asm__("$10") = 0xB0;
    unsigned char **ret;
    __asm__ volatile("jalr %1" : "=v0"(ret) : "r"(t2), "r"(t1));
    return ret;
}

void func_800C5BE4(void)
{
    unsigned char **b0_table;
    unsigned int *dst;
    unsigned int *src;
    volatile unsigned int old_word;

    func_800AD3D8();
    b0_table = bios_GetB0Table();
    dst = (unsigned int *)(b0_table[0x16C / 4] + 0x9C8);
    old_word = *dst;
    (void)old_word;

    for (src = D_800C5B3C; src != func_800C5B50; src++, dst++) {
        *dst = *src;
    }

    func_800AD4A8();
}
