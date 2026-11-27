/*
 * func_800655BC - Find a valid pass target teammate within angle and distance constraints
 *
 * Purpose:
 *   Iterates over 10 candidate teammates (in forward or reverse order at random),
 *   filtering by active flag, self-exclusion, Scratchpad 0x2AB avoidance, distance
 *   bounds, predicted-position bearing within tolerance, goal-line x-limit, and
 *   pass-lane clearance (func_80065440). Returns the first valid target's actor ID,
 *   or 0xFF if none is found.
 *
 * Arguments:
 *   actor           (u8*) - pointer to 1000-byte passer actor struct
 *   base_angle      (u8)  - reference pass heading (0..255)
 *   mode            (u8)  - pass type selector controlling clearance width & penalty:
 *                           0    -> width 0x10, penalty 0x000000
 *                           1    -> width 0x18, penalty 0x010000
 *                           2    -> width 0x1C, penalty 0x040000
 *                           4    -> width 0x24, penalty 0x310000
 *                           0xFF -> width 0x04, penalty 0x000000
 *                           else -> width 0x1C, penalty 0x100000
 *   angle_tolerance (u8)  - maximum signed/wrapped angular deviation allowed
 *
 * Return value:
 *   u32 - actor ID of chosen pass target, or 0xFF if no valid target found.
 *
 * Scratchpad / Actor fields read:
 *   Scratch 0x2AB (u8)  - actor ID to probabilistically skip (if func_8001E6B8(3) != 0)
 *   Actor +0x00   (u8)  - active flag
 *   Actor +0x02   (s16) - x position
 *   Actor +0x06   (s16) - z position
 *   Actor +0x08   (s32) - x velocity (used with 2x lead for target prediction)
 *   Actor +0x10   (s32) - z velocity (used with 2x lead for target prediction)
 *   Actor +0x38D  (u8)  - actor ID
 *
 * Magic numbers:
 *   dist_sq range (0xA0000, 0x4000000): ~3.2 m to 32.8 m (250 raw = 1 m)
 *   0x2E09 (11785): ~47.1 m max |x| for candidate target
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];

extern int func_8001E6B8(int range);
extern unsigned int func_8005CE14(unsigned int actor_id);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_80065440(unsigned char *actor, unsigned int angle,
                                  unsigned int width, unsigned int dist_sq);

unsigned int func_800655BC(unsigned char *actor, unsigned char base_angle,
                           unsigned int mode, unsigned int angle_tolerance)
{
    unsigned int width;
    unsigned int penalty;
    unsigned char reverse;
    unsigned int base_idx;
    unsigned char *cand;
    unsigned int count;
    unsigned int tol = angle_tolerance & 0xFF;

    mode &= 0xFF;
    if (mode == 2) {
        width = 0x1C;
        penalty = 0x040000;
    } else if (mode < 3) {
        if (mode == 0) {
            width = 0x10;
            penalty = 0;
        } else if (mode == 1) {
            width = 0x18;
            penalty = 0x010000;
        } else {
            width = 0x1C;
            penalty = 0x100000;
        }
    } else if (mode == 4) {
        width = 0x24;
        penalty = 0x310000;
    } else if (mode == 0xFF) {
        width = 0x04;
        penalty = 0;
    } else {
        width = 0x1C;
        penalty = 0x100000;
    }

    reverse = (unsigned char)func_8001E6B8(2);
    base_idx = func_8005CE14(B(actor, 0x38D)) & 0xFF;

    if (reverse == 0) {
        cand = D_800FF748 + base_idx * 0x3E8 + 0x3E8;
    } else {
        cand = D_800FF748 + base_idx * 0x3E8 + 0x2710;
    }

    for (count = 0; (count & 0xFF) < 10; count++) {
        if (B(cand, 0x00) != 0 && actor != cand) {
            int skip = 0;
            if (B(cand, 0x38D) == B(Scratchpad, 0x2AB)) {
                if (func_8001E6B8(3) != 0) {
                    skip = 1;
                }
            }

            if (!skip) {
                int ax = S(actor, 0x02);
                int az = S(actor, 0x06);
                int cx = S(cand, 0x02);
                int cz = S(cand, 0x06);
                int dx = ax - cx;
                int dz = az - cz;
                unsigned int dist_sq = (unsigned int)(dx * dx + dz * dz);

                if ((unsigned int)(dist_sq + 0xFFF5FFFFU) <= 0x3F5FFFEU) {
                    int pred_x = (signed short)(cx + (SW(cand, 0x08) << 1));
                    int pred_z = (signed short)(cz + (SW(cand, 0x10) << 1));
                    unsigned int angle = func_8005B36C(ax, az, pred_x, pred_z);
                    unsigned int diff = (angle - base_angle) & 0xFF;
                    int in_cone;

                    if (diff < 0x81) {
                        in_cone = ((int)diff < (int)tol);
                    } else {
                        in_cone = ((int)((base_angle - angle) & 0xFF) < (int)tol);
                    }

                    if (in_cone) {
                        int abs_cx = (cx >= 0) ? cx : -cx;
                        unsigned int cand_id = B(cand, 0x38D);
                        unsigned int lane_ok;

                        if (abs_cx >= 0x2E09) {
                            cand_id = 0xFF;
                        }

                        lane_ok = func_80065440(actor, angle & 0xFF, width & 0xFF, dist_sq + penalty);
                        if ((lane_ok & 0xFF) == 0) {
                            cand_id = 0xFF;
                        }

                        if ((cand_id & 0xFF) != 0xFF) {
                            return cand_id & 0xFF;
                        }
                    }
                }
            }
        }

        if (reverse == 0) {
            cand += 0x3E8;
        } else {
            cand -= 0x3E8;
        }
    }

    return 0xFF;
}
