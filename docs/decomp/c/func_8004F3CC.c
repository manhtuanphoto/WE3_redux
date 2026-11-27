/*
 * func_8004F3CC
 *
 * Purpose:
 *   Converts the remaining match-half countdown timer at Scratchpad+0x2C4 into
 *   scaled match-clock display ticks (0..162000 for regular halves, 0..54000
 *   for extra-time halves) based on the match-length setting (D_8010A5A8, 0..5)
 *   and period index (Scratchpad+0x2DF).
 *
 * Arguments:
 *   None
 *
 * Return value:
 *   (signed int) scaled elapsed match clock value
 *
 * Reads / Writes:
 *   D_8010A5A8: (u8) match duration option index (0..5) (R)
 *   Scratchpad (0x1F800000):
 *     +0x2C4 (s32) remaining half timer (R)
 *     +0x2DF (u8)  current period (< 2: regular half, >= 2: extra time) (R)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_8010A5A8;

int func_8004F3CC(void) {
    unsigned int mode = D_8010A5A8;
    unsigned char period;
    int timer;
    int base;
    int diff;
    int result;

    if (mode >= 6) {
        /* UNCLEAR: When D_8010A5A8 >= 6, assembly branches directly to jr $ra
         * with $v0 = $v1 (uninitialized $v1 in this function).
         */
        return 0;
    }

    period = B(Scratchpad, 0x2DF);
    timer = SW(Scratchpad, 0x2C4);

    switch (mode) {
    case 0:
        base = (period < 2) ? 0x2328 : 0x0BB8; /* 9000 / 3000 */
        diff = base - timer;
        result = diff * 18;
        break;
    case 1:
        base = (period < 2) ? 0x4650 : 0x1770; /* 18000 / 6000 */
        diff = base - timer;
        result = diff * 9;
        break;
    case 2:
        base = (period < 2) ? 0x6978 : 0x2328; /* 27000 / 9000 */
        diff = base - timer;
        result = diff * 6;
        break;
    case 3:
        base = (period < 2) ? 0x8CA0 : 0x2EE0; /* 36000 / 12000 */
        diff = base - timer;
        result = (diff * 9) / 2;
        break;
    case 4:
        base = (period < 2) ? 0xAFC8 : 0x3A98; /* 45000 / 15000 */
        diff = base - timer;
        result = (diff * 18) / 5;
        break;
    case 5:
        base = (period < 2) ? 0xD2F0 : 0x4650; /* 54000 / 18000 */
        diff = base - timer;
        result = diff * 3;
        break;
    default:
        result = 0;
        break;
    }

    return result;
}
