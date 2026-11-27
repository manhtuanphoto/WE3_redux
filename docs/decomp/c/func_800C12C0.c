/*
 * func_800C12C0
 *
 * Purpose:
 *   One-time initialization of the SPU/audio BIOS event and callback state
 *   guarded by D_800D557C. If D_800D557C == 0, sets D_800D557C = 1, enters a
 *   critical section (func_800AD3D8), initializes audio state at D_800C1778 via
 *   func_800C1E78, opens PS1 BIOS event 0xF0000009 (SPU DMA/IRQ event, spec
 *   0x20, mode 0x2000 = EvMdNOINTR, callback NULL) via OpenEvent (func_800ACE38),
 *   saves the event handle to D_800D5114, enables it via EnableEvent
 *   (func_800ACE58), and exits the critical section (func_800ACE68).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800D557C (u32, R/W): initialization guard flag (set to 1)
 *   D_800D5114 (u32, W):   opened PS1 BIOS event handle
 *   D_800C1778 (u8[], R):  audio/SPU init table or struct passed to func_800C1E78
 */

extern unsigned int D_800D557C;
extern unsigned int D_800D5114;
extern unsigned char D_800C1778[];

extern void func_800AD3D8(void); /* EnterCriticalSection (syscall 1) */
extern void func_800C1E78(unsigned char *arg);
extern unsigned int func_800ACE38(unsigned int desc, int spec, int mode, void (*func)(void)); /* OpenEvent (B0:08) */
extern void func_800ACE58(unsigned int event); /* EnableEvent (B0:0C) */
extern void func_800ACE68(void); /* ExitCriticalSection (syscall 2) */

void func_800C12C0(void)
{
    unsigned int event;

    if (D_800D557C != 0) {
        return;
    }

    D_800D557C = 1;
    func_800AD3D8();
    func_800C1E78(D_800C1778);

    event = func_800ACE38(0xF0000009u, 0x20, 0x2000, (void (*)(void))0);
    D_800D5114 = event;
    func_800ACE58(event);

    func_800ACE68();
}
