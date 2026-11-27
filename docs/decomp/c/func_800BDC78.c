/*
 * func_800BDC78
 *
 * Purpose:
 *   Clears a sound/channel slot (0..23) guarded by the reentrancy lock
 *   D_800F1014. If D_800F1014 is already 1, returns -1 immediately. Otherwise
 *   sets D_800F1014 = 1, validates ((unsigned short)slot < 0x18), writes slot
 *   to D_8010A3E2, calls func_800BFD18() to clear the slot state and bitmasks,
 *   clears the lock, and returns 0 (or -1 on invalid slot).
 *
 * Arguments:
 *   slot (int) - channel/slot index (validated as (unsigned short)slot < 24)
 *
 * Return value:
 *   int - 0 on success, -1 if locked (D_800F1014 == 1) or slot >= 24
 *
 * Global fields read/written:
 *   D_800F1014 (u32, R/W): reentrancy guard flag (1 while active)
 *   D_8010A3E2 (u16, W):   active slot index passed to func_800BFD18
 */

extern unsigned int D_800F1014;
extern unsigned short D_8010A3E2;

extern void func_800BFD18(void); /* asm passes $a0 = 0 in delay slot */

int func_800BDC78(int slot)
{
    if (D_800F1014 == 1) {
        return -1;
    }

    D_800F1014 = 1;
    if ((unsigned int)(slot & 0xFFFF) >= 0x18u) {
        D_800F1014 = 0;
        return -1;
    }

    D_8010A3E2 = (unsigned short)slot;
    func_800BFD18();
    D_800F1014 = 0;
    return 0;
}
