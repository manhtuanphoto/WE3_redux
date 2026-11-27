/*
 * func_800C5B50
 *
 * Purpose:
 *   Patches the PS1 BIOS C0 exception/card-related handler using the code
 *   template D_800C5B28..D_800C5B3C. Enters critical section (func_800AD3D8),
 *   invokes BIOS B0:56 (GetC0Table), reads the base pointer at c0_table[6]
 *   (+0x18), decodes a 32-bit target address from the lui/addiu-style
 *   instructions at (+0x70, +0x74), adds 0x28, copies D_800C5B28..D_800C5B3C
 *   into that location, stores the resulting end pointer to 0x0000DFFC
 *   (0x10000 - 0x2004), and flushes the instruction cache (func_800AD4A8).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields referenced:
 *   D_800E7328 (u32, W): saves/restores $ra without a stack frame
 *   D_800C5B28..D_800C5B3C (const u32[]): patch instruction sequence
 *   *(u32 *)0x0000DFFC (W): updated with end pointer after patch copy
 */

extern unsigned int D_800E7328;
extern const unsigned int D_800C5B28[];
extern const unsigned int D_800C5B3C[];

extern void func_800AD3D8(void); /* EnterCriticalSection (syscall 1) */
extern void func_800AD4A8(void); /* FlushCache (BIOS A0:44) */

/* PS1 BIOS B0:56 (GetC0Table) */
static unsigned int *ps1_bios_get_c0_table(void)
{
    typedef unsigned int *(*bios_b0_fn)(void);
    /* UNCLEAR: BIOS dispatch at 0x000000B0 with $t1 = 0x56; cannot express $t1 in standard C */
    return ((bios_b0_fn)0x000000B0)();
}

void func_800C5B50(void)
{
    unsigned int *c0_table;
    unsigned int *handler;
    unsigned int hi16;
    unsigned int lo16;
    unsigned int *dst;
    const unsigned int *src;
    const unsigned int *end;

    /* Assembly saves $ra in D_800E7328 without a stack frame */
    func_800AD3D8();

    c0_table = ps1_bios_get_c0_table();
    handler = (unsigned int *)c0_table[0x18 / 4];

    hi16 = handler[0x70 / 4] & 0xFFFFu;
    lo16 = handler[0x74 / 4] & 0xFFFFu;
    dst = (unsigned int *)((hi16 << 16) + lo16 + 0x28u);

    src = D_800C5B28;
    end = D_800C5B3C;
    while (src != end) {
        *dst++ = *src++;
    }

    *(volatile unsigned int *)0x0000DFFC = (unsigned int)dst;
    func_800AD4A8();
}
