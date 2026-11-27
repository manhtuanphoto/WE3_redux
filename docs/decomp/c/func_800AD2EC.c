/*
 * func_800AD2EC
 *
 * Purpose:
 *   Clears the 16-bit hardware register at *(D_800CEA68 + 0xA) to 0 and runs a
 *   short busy-wait delay loop (counting a stack-allocated volatile counter
 *   from 10 down to -1), then returns 0.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   int: 0
 *
 * Global fields read:
 *   D_800CEA68 (volatile u8 *): hardware register block base (e.g., SIO0/1)
 */

extern volatile unsigned char *D_800CEA68;

int func_800AD2EC(void)
{
    volatile int delay;

    *(volatile unsigned short *)(D_800CEA68 + 0xA) = 0;

    delay = 10;
    while (--delay != -1) {
    }

    return 0;
}
