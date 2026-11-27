/*
 * func_80051810
 *
 * Purpose:
 *   Camera / replay-view dispatcher by match phase (Scratchpad+0x29A, valid
 *   range 1..13 via jtbl_80011D40). Updates Scratchpad camera eye/target
 *   vectors (+0x1F0..+0x20F and backup +0x210..+0x22F) or delegates to
 *   sub-handlers func_80051A84 / func_80096xxx / func_80097xxx.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   jtbl_80011D40 (jump table in .rodata for phase - 1 in 0..12)
 *
 * Scratchpad (0x1F800000) fields read/written:
 *   +0x1E8 (u8)  cleared in first jump-table case
 *   +0x1EC (u8)  cleared in .L80051994 when +0x29B < 2
 *   +0x1F0..+0x20F (s32[8]) camera eye/target coordinates (R/W)
 *   +0x210..+0x22F (s32[8]) saved/previous camera coordinates (W)
 *   +0x29A (u8)  match phase (1..13)
 *   +0x29B (u8)  match sub-phase
 *   +0x2E7 (u8)  camera azimuth angle (W: 0xC0)
 *   +0x2F4 (u32) ball pointer (reads +0x02 X, +0x04 Y, +0x06 Z)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern void func_80051A84(unsigned char *cam);
extern void func_800975A0(void);
extern void func_80096808(void);
extern void func_80096094(unsigned int mode);
extern void func_800972AC(void);
extern void func_80096964(void);
extern void func_80097C04(void);
extern void func_8009787C(void);
extern void func_80097DB0(void);
extern signed int func_8001E694(unsigned char ang, int radius);
extern signed int func_8001E5A4(unsigned char ang, int radius);

void func_80051810(void)
{
    unsigned char phase = B(Scratchpad, 0x29A);
    unsigned int idx = (unsigned int)(phase - 1);

    if (idx >= 0xDU) {
        return;
    }

    switch (idx) {
    case 0:
    case 11: /* .L80051850 */
        B(Scratchpad, 0x1E8) = 0;
        func_80051A84(Scratchpad + 0x1F0);
        break;

    case 1: /* .L80051858 */
        func_800975A0();
        break;

    case 2: /* .L80051868 */
        if (B(Scratchpad, 0x29B) != 0) {
            int i;
            func_80096808();
            for (i = 0; i < 32; i += 4) {
                W(Scratchpad, 0x210 + i) = W(Scratchpad, 0x1F0 + i);
            }
        }
        break;

    case 3: /* .L800518C8 */
        if (B(Scratchpad, 0x29B) != 0) {
            func_80096094(1);
        }
        break;

    case 4: /* .L80051A70 */
        break;

    case 5: /* .L800518E8 */
        if (B(Scratchpad, 0x29B) != 0) {
            func_800972AC();
        }
        break;

    case 6: /* .L80051908 */
        if (B(Scratchpad, 0x29B) != 0) {
            func_80096964();
        }
        break;

    case 7: /* .L80051928 */
        if (B(Scratchpad, 0x29B) < 2) {
            SW(Scratchpad, 0x1F0) = 0x40;
            SW(Scratchpad, 0x1F4) = -0xA00;
            SW(Scratchpad, 0x1F8) = -0x2400;
            SW(Scratchpad, 0x1FC) = 0x40;
            SW(Scratchpad, 0x200) = -0x100;
            SW(Scratchpad, 0x204) = 0;
        } else {
            func_80097C04();
        }
        break;

    case 8: /* .L80051974 */
        if (B(Scratchpad, 0x29B) != 0) {
            func_80096094(2);
        }
        break;

    case 9: /* .L80051994 */
        if (B(Scratchpad, 0x29B) < 2) {
            B(Scratchpad, 0x1EC) = 0;
            func_80051A84(Scratchpad + 0x1F0);
        } else {
            func_8009787C();
        }
        break;

    case 10: /* .L800519C8 */
        if (B(Scratchpad, 0x29B) < 3) {
            func_80097DB0();
        }
        break;

    case 12: /* .L800519EC */
        if (B(Scratchpad, 0x29B) != 0) {
            unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
            signed int bx = S(ball, 2);
            signed int by = S(ball, 4);
            signed int bz = S(ball, 6);
            SW(Scratchpad, 0x1FC) = bx;
            SW(Scratchpad, 0x200) = by;
            SW(Scratchpad, 0x204) = bz;
            B(Scratchpad, 0x2E7) = 0xC0;
            SW(Scratchpad, 0x1F0) = bx + func_8001E694(0xC0, 0x2000);
            SW(Scratchpad, 0x1F4) = -0xC00;
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            SW(Scratchpad, 0x1F8) = S(ball, 6) + func_8001E5A4(B(Scratchpad, 0x2E7), 0x2000);
        }
        break;

    default:
        break;
    }
}
