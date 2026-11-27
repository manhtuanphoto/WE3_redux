/*
 * func_800A8F3C
 *
 * Purpose:
 *   Ball perimeter/adboard collision and bounce handler. Selects the Z-boundary
 *   half-extent from Scratchpad+0x2E3 via jump table jtbl_80014AA0 (values
 *   0x2A20, 0x2710, 0x28A0, 0x2580). When ball height (ball+0x4) >= -256:
 *   - If |x| <= 0x3AA0 and (z_bound - 0xA0) < |z| < (z_bound + 0xA0), clamps z
 *     to ±(z_bound ± 0xA0), negates ball heading (+0x3A4), and halves +0x3A0.
 *   - Else if |z| <= 0x248C and 0x39F1 <= |x| <= 0x3B2F, clamps x to ±0x39F0 or
 *     ±0x3B30, reflects heading (+0x3A4 = 0x80 - heading), and halves +0x3A0.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x2E3 (u8) stadium/boundary mode (0..5)
 *   +0x2F4 (u8 *) pointer to ball actor struct:
 *     +0x002 (s16) ball x
 *     +0x004 (s16) ball y (height)
 *     +0x006 (s16) ball z
 *     +0x3A0 (u32) ball speed/momentum (halved on bounce)
 *     +0x3A4 (u8) ball heading angle (0..255)
 *     +0x3C2 (s16) previous ball x
 *     +0x3C6 (s16) previous ball z
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define SH(p, o) (*(short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

/*
 * UNCLEAR: jtbl_80014AA0 table entries for cases 0..5 are not present in this
 * .s file. Observed branch targets inside func_800A8F3C set z_bound to:
 *   .L800A8FB8 -> 0x2A20
 *   .L800A8FC0 -> 0x2710
 *   .L800A8FC8 -> 0x28A0
 *   .L800A8FD0 -> 0x2580
 *   (and possibly .L800A9120 -> early return)
 */
extern void *jtbl_80014AA0[];

void func_800A8F3C(void)
{
    unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
    short bx = SH(ball, 0x002);
    short bz = SH(ball, 0x006);
    short px = SH(ball, 0x3C2);
    short pz = SH(ball, 0x3C6);
    int abs_x = (bx >= 0) ? (int)bx : -(int)bx;
    int abs_z = (bz >= 0) ? (int)bz : -(int)bz;
    int abs_px = (px >= 0) ? (int)px : -(int)px;
    int abs_pz = (pz >= 0) ? (int)pz : -(int)pz;
    unsigned char mode = B(Scratchpad, 0x2E3);
    int z_bound;

    if (mode >= 6) {
        return;
    }

    /* UNCLEAR: exact mapping of jtbl_80014AA0[0..5] to the 4 boundary constants */
    switch ((unsigned int)(unsigned long)jtbl_80014AA0[mode]) {
    case 0:
        z_bound = 0x2A20;
        break;
    case 1:
        z_bound = 0x2710;
        break;
    case 2:
        z_bound = 0x28A0;
        break;
    case 3:
        z_bound = 0x2580;
        break;
    default:
        return;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    if (SH(ball, 0x004) < -0x100) {
        return;
    }

    if ((unsigned int)(abs_x & 0xFFFF) < 0x3AA1U) {
        int z_lo = (z_bound & 0xFFFF) - 0xA0;
        int z_hi = (z_bound & 0xFFFF) + 0xA0;
        int az = abs_z & 0xFFFF;

        if (az > z_lo && az < z_hi) {
            short new_z;
            if ((unsigned int)(abs_pz & 0xFFFF) < (unsigned int)az) {
                new_z = (SH(ball, 0x006) >= 0) ? (short)z_lo : (short)(-z_lo);
            } else {
                new_z = (SH(ball, 0x006) >= 0) ? (short)z_hi : (short)(-z_hi);
            }
            SH(ball, 0x006) = new_z;

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            B(ball, 0x3A4) = (unsigned char)(-(int)B(ball, 0x3A4));

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            W(ball, 0x3A0) >>= 1;
            return;
        }
    }

    if ((unsigned int)(abs_z & 0xFFFF) < 0x248DU) {
        unsigned int ax = (unsigned int)(abs_x & 0xFFFF);
        if ((unsigned int)((ax - 0x39F1U) & 0xFFFF) < 0x13FU) {
            short new_x;
            if ((unsigned int)(abs_px & 0xFFFF) < ax) {
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                new_x = (SH(ball, 0x002) >= 0) ? (short)0x39F0 : (short)-0x39F0;
            } else {
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                new_x = (SH(ball, 0x002) >= 0) ? (short)0x3B30 : (short)-0x3B30;
            }
            SH(ball, 0x002) = new_x;

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            B(ball, 0x3A4) = (unsigned char)(0x80 - (int)B(ball, 0x3A4));

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            W(ball, 0x3A0) >>= 1;
        }
    }
}
