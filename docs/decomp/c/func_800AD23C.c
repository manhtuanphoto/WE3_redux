/*
 * func_800AD23C
 *
 * Purpose:
 *   Initializes and registers the priority-1 BIOS interrupt chain node at
 *   D_800E7008 (PsyQ ResetCallback / interrupt dispatcher setup). Enters a
 *   critical section, populates D_800E7008..D_800E7014 with handler callbacks
 *   func_800AD2EC and func_800AD354, dequeues and re-enqueues the node on IRQ
 *   priority 1 via SysDeqIntRP (func_800AD3F8) and SysEnqIntRP (func_800AD3E8),
 *   exits the critical section, and returns 1.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   int - returns 1 (set in delay slot of ExitCriticalSection syscall func_800ACE68)
 *
 * Global fields written:
 *   D_800E7008 (u32) = 0 (IntRP next link)
 *   D_800E700C (u32) = (u32)func_800AD2EC (IntRP handler 1)
 *   D_800E7010 (u32) = (u32)func_800AD354 (IntRP handler 2)
 *   D_800E7014 (u32) = 0
 */

extern unsigned int D_800E7008;
extern unsigned int D_800E700C;
extern unsigned int D_800E7010;
extern unsigned int D_800E7014;

extern void func_800AD2EC(void);
extern void func_800AD354(void);
extern void func_800AD3D8(void); /* EnterCriticalSection (syscall 1) */
extern void func_800AD3E8(int priority, unsigned int *node); /* SysEnqIntRP (BIOS C0:02) */
extern void func_800AD3F8(int priority, unsigned int *node); /* SysDeqIntRP (BIOS C0:03) */
extern void func_800ACE68(void); /* ExitCriticalSection (syscall 2) */

int func_800AD23C(void)
{
    unsigned int *node = &D_800E7008;

    func_800AD3D8();

    D_800E700C = (unsigned int)func_800AD2EC;
    D_800E7010 = (unsigned int)func_800AD354;
    D_800E7008 = 0;
    D_800E7014 = 0;

    func_800AD3F8(1, node);
    func_800AD3E8(1, node);

    /* Note: asm sets $v0 = 1 in delay slot of jal func_800ACE68 (syscall 2 preserves $v0) */
    func_800ACE68();
    return 1;
}
