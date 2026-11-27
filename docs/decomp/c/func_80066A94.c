/*
 * func_80066A94
 *
 * Purpose:
 *   Per-frame match/gameplay tick scheduler. Calls func_80066BD8(2 or 0xD)
 *   alternating on Scratchpad+0x290 & 1, dispatches staggered sub-updates on
 *   (Scratchpad+0x290 % 5) via jtbl_80012500 (including a every-20-frames call
 *   to func_800A27C0), increments ball-flight timer Scratchpad+0x31C up to 0xFF,
 *   and calls func_800680E8, func_80068474, and func_80067FAC.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Scratchpad (0x1F800000) fields read/written:
 *   +0x290 (signed int, R): match/game frame counter
 *   +0x31C (signed short / unsigned short, R/W): frames since last ball touch
 *          (incremented each tick while != 0xFF; capped at 255)
 *
 * Notes:
 *   UNCLEAR: Exact index mapping of jtbl_80012500[0..4] across the four jlabels
 *   (.L80066B14, .L80066B58, .L80066B68, .L80066B80) and exit (.L80066B90).
 *   Case 0 must be .L80066B14 because `frame % 20 == 0` implies `frame % 5 == 0`.
 */

#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define SH(p, o) (*(signed short *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern void func_80066BD8(unsigned int arg);
extern void func_800A27C0(void);
extern void func_800A03FC(void);
extern void func_8009FFB0(void);
extern void func_800A18D4(void);
extern void func_80067D10(void);
extern void func_80067B20(void);
extern void func_800680E8(void);
extern void func_80068474(void);
extern void func_80067FAC(void);

void func_80066A94(void)
{
    signed int frame;
    signed int slot;

    func_80066BD8((W(Scratchpad, 0x290) & 1u) ? 2u : 0xDu);

    frame = SW(Scratchpad, 0x290);
    slot = frame % 5;

    if ((unsigned int)slot < 5u) {
        /*
         * UNCLEAR: exact jtbl_80012500[0..4] mapping for cases 1..4
         * (one of the 5 entries jumps straight to exit .L80066B90 or duplicates a label).
         * Case 0 is .L80066B14 because frame % 20 == 0 requires frame % 5 == 0.
         */
        switch (slot) {
        case 0: /* .L80066B14 */
            if ((SW(Scratchpad, 0x290) % 20) == 0) {
                func_800A27C0();
            }
            break;
        case 1: /* .L80066B58 (order in .s; exact jtbl_80012500 slot UNCLEAR) */
            func_800A03FC();
            break;
        case 2: /* .L80066B68 (order in .s; exact jtbl_80012500 slot UNCLEAR) */
            func_8009FFB0();
            func_800A18D4();
            break;
        case 3: /* .L80066B80 (order in .s; exact jtbl_80012500 slot UNCLEAR) */
            func_80067D10();
            func_80067B20();
            break;
        case 4:
            break;
        }
    }

    if (SH(Scratchpad, 0x31C) != 0xFF) {
        H(Scratchpad, 0x31C) = (unsigned short)(H(Scratchpad, 0x31C) + 1);
    }

    func_800680E8();
    func_80068474();
    func_80067FAC();
}
