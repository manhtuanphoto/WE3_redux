/*
 * func_800BA518
 *
 * Purpose:
 *   PS1 VSync-style vertical blank synchronization and root-counter timing helper.
 *   Reads a hardware counter via pointer D_800D3D80 (double-read until stable) to
 *   compute elapsed 16-bit ticks since D_800D3D84, and either queries the VSync
 *   count (mode < 0), returns elapsed ticks immediately (mode == 1), or waits
 *   for VSyncs via func_800BA690 and optional interlace field toggle on *D_800D3D7C.
 *
 * Arguments:
 *   mode (signed int):
 *     < 0 : return current VSync counter D_800D4EB4
 *     == 1: return 16-bit elapsed counter delta without waiting
 *     == 0: wait for next VSync (calls func_800BA690(D_800D3D88, 0))
 *     >= 2: wait for mode VSyncs (calls func_800BA690(D_800D3D88 + mode - 1, mode - 1))
 *
 * Return value:
 *   D_800D4EB4 if mode < 0; otherwise 16-bit elapsed counter ticks (u16).
 *
 * Global fields read / written:
 *   D_800D3D7C (volatile unsigned int *): hardware status register (bit 22 = 0x400000, bit 31 = 0x80000000; likely GPUSTAT)
 *   D_800D3D80 (volatile unsigned int *): hardware counter register (double-read for stability)
 *   D_800D3D84 (unsigned int): previous hardware counter snapshot
 *   D_800D3D88 (unsigned int): target/last VSync counter snapshot
 *   D_800D4EB4 (unsigned int): current VSync frame counter
 */

extern volatile unsigned int *D_800D3D7C;
extern volatile unsigned int *D_800D3D80;
extern unsigned int D_800D3D84;
extern unsigned int D_800D3D88;
extern unsigned int D_800D4EB4;

extern void func_800BA690(unsigned int target, signed int arg1);

unsigned int func_800BA518(signed int mode)
{
    volatile unsigned int *statusReg = D_800D3D7C;
    volatile unsigned int *counterReg = D_800D3D80;
    unsigned int initialStatus = *statusReg;
    unsigned int counterSnap;
    unsigned int elapsed;

    do {
        counterSnap = *counterReg;
    } while (counterSnap != *counterReg);

    elapsed = (counterSnap - D_800D3D84) & 0xFFFF;

    if (mode < 0) {
        return D_800D4EB4;
    }

    if (mode == 1) {
        return elapsed;
    }

    if (mode <= 0) {
        func_800BA690(D_800D3D88, 0);
    } else {
        func_800BA690(D_800D3D88 + (unsigned int)mode - 1U, mode - 1);
    }

    initialStatus = *D_800D3D7C;
    func_800BA690(D_800D4EB4 + 1U, 1);

    if (initialStatus & 0x400000U) {
        volatile unsigned int *pollReg = D_800D3D7C;
        if ((signed int)(initialStatus ^ *pollReg) >= 0) {
            while (((*pollReg ^ initialStatus) & 0x80000000U) == 0) {
                /* Wait for bit 31 (interlace field) to toggle */
            }
        }
    }

    counterReg = D_800D3D80;
    D_800D3D88 = D_800D4EB4;

    do {
        D_800D3D84 = *counterReg;
    } while (D_800D3D84 != *counterReg);

    return elapsed;
}
