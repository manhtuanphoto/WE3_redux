/*
 * func_8006F0DC
 *
 * Purpose:
 *   Screen fade-out / brightness ramp step and completion handler. Marks
 *   Scratchpad+0x330 = 1 while active, ramps D_8010A31E up by (step & 0xFF)
 *   clamped to 0xFF, and once D_8010A31E == 0xFF executes the requested
 *   completion action (`mode & 0xFF`: 1, 2, or 3).
 *
 * Arguments:
 *   step (unsigned int): fade increment per call (masked to 0xFF)
 *   mode (unsigned int): completion action when D_8010A31E == 0xFF (masked to 0xFF):
 *                        1 = call func_8004FC54(0); if game state == 4 call func_8018E538(0) & clear +0x2A2
 *                        2 = clear Scratchpad+0x330 = 0
 *                        3 = both action 1 and clear Scratchpad+0x330 = 0
 *
 * Return value:
 *   1 if D_8010A31E had already reached 0xFF on entry;
 *   0 while D_8010A31E is still ramping up toward 0xFF.
 *
 * Global fields read/written:
 *   D_8010A31E (signed short): fade level (0..0xFF)
 *
 * Scratchpad (0x1F800000) fields read/written:
 *   +0x298 (unsigned char, R): game state (4 triggers func_8018E538(0))
 *   +0x2A2 (unsigned char, W): cleared to 0 when game state == 4 in mode 1/3
 *   +0x330 (unsigned char, W): set to 1 on entry; cleared to 0 in mode 2/3
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern signed short D_8010A31E;

extern void func_8004FC54(unsigned int a0);
extern void func_8018E538(unsigned int a0);

int func_8006F0DC(unsigned int step, unsigned int mode)
{
    signed short cur = D_8010A31E;
    unsigned int step_byte = step & 0xFFu;
    unsigned int m;

    B(Scratchpad, 0x330) = 1;

    if (cur == 0xFF) {
        m = mode & 0xFFu;
        if (m == 1u) {
            func_8004FC54(0u);
            if (B(Scratchpad, 0x298) == 4u) {
                func_8018E538(0u);
                B(Scratchpad, 0x2A2) = 0;
            }
        } else if (m == 2u) {
            B(Scratchpad, 0x330) = 0;
        } else if (m == 3u) {
            func_8004FC54(0u);
            if (B(Scratchpad, 0x298) == 4u) {
                func_8018E538(0u);
                B(Scratchpad, 0x2A2) = 0;
            }
            B(Scratchpad, 0x330) = 0;
        }
        return 1;
    }

    if ((int)(cur + step_byte) < 0xFF) {
        D_8010A31E = (signed short)(cur + step_byte);
    } else {
        D_8010A31E = 0xFF;
    }

    return 0;
}
