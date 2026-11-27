/*
 * func_80066150 - Evaluate whether a wing/flank pass or cross-field switch to target is viable
 *
 * Purpose:
 *   Checks spatial geometry and pass-lane clearance between `actor` (passer) and
 *   `target` (receiver on the wing: 0x12F4 <= |target.z| < 0x1E01). Verifies
 *   x/z bounds, squared distance in (0x8FFFF, 0x4000000], forward progress along
 *   attack direction, two pass-lane checks via func_80065440, and a final
 *   receiver-side check scaled by |dz| bucket and player stat nibble (D_8010C32E).
 *
 * Arguments:
 *   actor  (u8*) - pointer to 1000-byte passer actor struct
 *   target (u8*) - pointer to 1000-byte candidate receiver actor struct
 *
 * Return value:
 *   unsigned int - 1 if viable, 0 otherwise.
 *
 * Globals / Actor fields read:
 *   D_8010C1D8    (u8)    - global disable/override flag (nonzero -> returns 0)
 *   D_8010C32E    (u16[]) - player stat entry (indexed by side*264 + slot*12, low nibble 0..15)
 *   Actor +0x02   (s16)   - x position
 *   Actor +0x06   (s16)   - z position
 *   Actor +0x392  (u8)    - team slot (0..10)
 *   Actor +0x398  (u8)    - side (0/1)
 *   Actor +0x399  (u8)    - attack direction flag (0 = +X, 1 = -X)
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define H(p, o) (*(unsigned short *)((p) + (o)))
#define S(p, o) (*(signed short *)((p) + (o)))

extern unsigned char D_8010C1D8;
extern unsigned char D_8010C32E[];

extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_80065440(unsigned char *actor, unsigned int angle,
                                  unsigned int width, unsigned int dist_sq);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int current, unsigned int step);

unsigned int func_80066150(unsigned char *actor, unsigned char *target)
{
    int ax, az, tx, tz;
    int abs_tz, abs_ax, dz, abs_dz, abs_tx;
    unsigned int dist_sq;
    signed short x_offset;
    unsigned int angle;
    unsigned int abs_dz2;
    unsigned int width_bonus;
    unsigned int stat;
    unsigned int res;

    if (actor == target) {
        return 0;
    }
    if (D_8010C1D8 != 0) {
        return 0;
    }

    tz = S(target, 0x06);
    abs_tz = (tz >= 0) ? tz : -tz;
    if (abs_tz >= 0x1E01 || abs_tz < 0x12F4) {
        return 0;
    }

    ax = S(actor, 0x02);
    abs_ax = (ax >= 0) ? ax : -ax;
    if (abs_ax >= 0x23E3) {
        return 0;
    }

    az = S(actor, 0x06);
    dz = az - tz;
    abs_dz = (dz >= 0) ? dz : -dz;
    if (abs_dz >= 0x1601) {
        return 0;
    }

    tx = S(target, 0x02);
    abs_tx = (tx >= 0) ? tx : -tx;
    if (abs_tx >= 0x27E3) {
        return 0;
    }

    dist_sq = (unsigned int)((ax - tx) * (ax - tx) + dz * dz);
    if (dist_sq > 0x4000000U || dist_sq <= 0x8FFFFU) {
        return 0;
    }

    {
        int abs_az = (az >= 0) ? az : -az;
        x_offset = (abs_tz < abs_az) ? -0x300 : 0x300;
    }

    if (B(actor, 0x399) == 0) {
        if ((tx - x_offset) < ax) {
            return 0;
        }
    } else {
        if (ax < (tx + x_offset)) {
            return 0;
        }
    }

    angle = func_8005B36C(ax, az, tx, tz);
    if ((func_80065440(actor, angle & 0xFF, 8, dist_sq) & 0xFF) == 0) {
        return 0;
    }

    {
        int raw_dz = S(actor, 0x06) - S(target, 0x06);
        unsigned int turned;

        abs_dz2 = (unsigned int)((raw_dz >= 0) ? raw_dz : -raw_dz);
        angle = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(target, 0x02), S(target, 0x06));
        turned = func_8005B7CC(angle & 0xFF, (B(actor, 0x399) << 7) & 0x80, 0x20);
        if ((func_80065440(actor, turned & 0xFF, 0x10, abs_dz2 * abs_dz2) & 0xFF) == 0) {
            return 0;
        }
    }

    if (abs_dz2 >= 0x1081U) {
        width_bonus = 0x20;
    } else if (abs_dz2 >= 0xB01U) {
        width_bonus = 0x10;
    } else if (abs_dz2 >= 0x841U) {
        width_bonus = 8;
    } else if (abs_dz2 >= 0x69AU) {
        width_bonus = 4;
    } else {
        width_bonus = 0;
    }

    stat = H(D_8010C32E, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) & 0xF;
    res = func_80065440(target,
                        (B(actor, 0x399) << 7) & 0x80,
                        (width_bonus + 0x21 - stat) & 0xFF,
                        ((dist_sq * 3U) >> 1) + 0x900000U);

    return ((res & 0xFF) != 0) ? 1U : 0U;
}
