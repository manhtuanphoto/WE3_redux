/*
 * func_800B48A8
 *
 * Purpose:
 *   Swaps/applies the active display buffer coordinates from `D_800E7800` and
 *   `D_800E789C` indexed by `D_8010CD54` into `D_800F0FD8` (`func_800AED64`,
 *   PutDispEnv-like), enables display via `func_800AE49C(1)`, increments frame
 *   counter `D_8010CD50` (skipping 0 on overflow), toggles `D_8010CD54`
 *   between 0 and 1, and invokes `func_800B47C8()` and `func_800B4698()`.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_8010CD54 (s16)   active display buffer index (0 or 1, toggled)
 *   D_800E7800 (u16[]) per-buffer display X coordinate
 *   D_800E789C (u16[]) per-buffer display Y coordinate
 *   D_800F0FD8 (u16[]) active DISPENV buffer (+0x00 X, +0x02 Y)
 *   D_8010CD50 (u32)   non-zero frame counter
 */

#define H(p, o) (*(unsigned short *)((p) + (o)))

extern signed short D_8010CD54;
extern unsigned short D_800E7800[];
extern unsigned short D_800E789C[];
extern unsigned char D_800F0FD8[];
extern unsigned int D_8010CD50;

extern void func_800AED64(unsigned char *env);
extern void func_800AE49C(unsigned int mask);
extern void func_800B47C8(void);
extern void func_800B4698(void);

void func_800B48A8(void)
{
    signed short idx = D_8010CD54;
    unsigned int frameCount;

    H(D_800F0FD8, 0x00) = D_800E7800[idx];
    H(D_800F0FD8, 0x02) = D_800E789C[idx];
    func_800AED64(D_800F0FD8);
    func_800AE49C(1U);

    frameCount = D_8010CD50 + 1U;
    D_8010CD50 = frameCount;
    if (frameCount == 0U) {
        frameCount = 1U;
    }
    D_8010CD50 = frameCount;

    D_8010CD54 = (D_8010CD54 == 0) ? 1 : 0;
    func_800B47C8();
    func_800B4698();
}
