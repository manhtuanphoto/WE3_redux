/*
 * func_80015B98
 *
 * Purpose:
 *   Per-frame timer/counter update and countdown handler. Increments Scratchpad+0x1A0,
 *   latches it to D_8010CDDC when it reaches Scratchpad+0x19C while Scratchpad+0x1A4 is
 *   nonzero, updates countdown timers at Scratchpad+0x2C4/+0x2C8 when Scratchpad+0x2C2 == 1
 *   and Scratchpad+0x2A0 == 0, and calls func_800BB248().
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields written:
 *   D_8010CDDC (u32)
 *
 * Scratchpad (0x1F800000) fields read/written:
 *   +0x19C (s32) threshold compared against +0x1A0
 *   +0x1A0 (s32) frame/tick counter
 *   +0x1A4 (u32) pending reset value / trigger flag (cleared to 0 on latch)
 *   +0x2A0 (u8)  pause/overlay active flag
 *   +0x2C2 (u8)  countdown active flag (1 = active, cleared to 0 when +0x2C8 reaches 0)
 *   +0x2C4 (s32) primary countdown timer
 *   +0x2C8 (s32) secondary countdown timer
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned int D_8010CDDC;

extern void func_800BB248(void);

void func_80015B98(void)
{
    SW(Scratchpad, 0x1A0) = SW(Scratchpad, 0x1A0) + 1;

    if (SW(Scratchpad, 0x1A0) >= SW(Scratchpad, 0x19C) && W(Scratchpad, 0x1A4) != 0) {
        D_8010CDDC = W(Scratchpad, 0x1A0);
        W(Scratchpad, 0x1A4) = 0;
        W(Scratchpad, 0x1A0) = W(Scratchpad, 0x1A4);
    }

    if (B(Scratchpad, 0x2C2) == 1 && B(Scratchpad, 0x2A0) == 0) {
        signed int t1 = SW(Scratchpad, 0x2C4);
        if (t1 > 0) {
            SW(Scratchpad, 0x2C4) = t1 - 1;
        } else {
            signed int t2 = SW(Scratchpad, 0x2C8) - 1;
            if (t2 > 0) {
                SW(Scratchpad, 0x2C8) = t2;
            } else {
                SW(Scratchpad, 0x2C8) = 0;
                B(Scratchpad, 0x2C2) = 0;
            }
        }
    }

    func_800BB248();
}
