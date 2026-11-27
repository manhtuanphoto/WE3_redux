/*
 * func_800AD4E4
 *
 * Purpose:
 *   Patches the PS1 BIOS B0:5B handler table entry (GetB0Table()[0x5B]).
 *   Enters critical section (func_800AD3D8), invokes BIOS B0:57 (GetB0Table),
 *   reads b0_table[0x5B] (offset +0x16C), records (base + 0x7A0) in
 *   D_800E702C, copies the patch instructions from func_800AD570..D_800AD580
 *   to both (base + 0x3D8) and (base + 0x4E0), flushes cache (func_800AD4A8),
 *   exits critical section (func_800ACE68), and returns D_800E702C.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   unsigned int - D_800E702C ((unsigned int)b0_handler + 0x7A0)
 *
 * Global fields written:
 *   D_800E7028 (u32): saves/restores $ra without a stack frame
 *   D_800E702C (u32): saved continuation address (b0_handler + 0x7A0)
 */

extern unsigned int D_800E7028;
extern unsigned int D_800E702C;
extern const unsigned int func_800AD570[];
extern const unsigned int D_800AD580[];

extern void func_800AD3D8(void); /* EnterCriticalSection (syscall 1) */
extern void func_800AD4A8(void); /* FlushCache (BIOS A0:44) */
extern void func_800ACE68(void); /* ExitCriticalSection (syscall 2) */

/* PS1 BIOS B0:57 (GetB0Table) */
static unsigned int *ps1_bios_get_b0_table(void)
{
    typedef unsigned int *(*bios_b0_fn)(void);
    /* UNCLEAR: BIOS dispatch at 0x000000B0 with $t1 = 0x57; cannot express $t1 in standard C */
    return ((bios_b0_fn)0x000000B0)();
}

unsigned int func_800AD4E4(void)
{
    unsigned int *b0_table;
    unsigned char *handler_base;
    const unsigned int *src;
    const unsigned int *end;

    /* Assembly saves $ra in D_800E7028 without a stack frame */
    func_800AD3D8();

    b0_table = ps1_bios_get_b0_table();
    handler_base = (unsigned char *)b0_table[0x16C / 4];
    D_800E702C = (unsigned int)(handler_base + 0x7A0);

    src = func_800AD570;
    end = D_800AD580;
    while (src != end) {
        unsigned int word = *src;
        *(unsigned int *)(handler_base + 0x3D8) = word;
        *(unsigned int *)(handler_base + 0x4E0) = word;
        handler_base += 4;
        src++;
    }

    func_800AD4A8();
    func_800ACE68();

    return D_800E702C;
}
