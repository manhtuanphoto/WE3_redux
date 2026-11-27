/*
 * func_800484F8 - Simulate ball trajectory up to 63 frames to predict intercept time
 *
 * Purpose:
 *   Saves the ball's current state from scratch 0x2F4 into scratchpad 0xB0..0xBC
 *   and 0xF8..0xFC, then steps the ball physics (func_8004BAA4) for t = 1..63 frames.
 *   Returns the frame count t (or first reachable frame) when the ball crosses pitch
 *   bounds or becomes reachable by a player at (target_x, target_z) moving at
 *   max_speed (clamped >= 0x30), subject to height and heading checks per mode.
 *   Also caches the ball height at that frame into ball+0x3BE if still 0.
 *
 * Arguments:
 *   target_x  - s16 target X coordinate
 *   target_z  - s16 target Z coordinate
 *   max_speed - u16 player speed per frame (clamped to >= 0x30 = 48 raw = 0.192 m/frame)
 *   mode      - u8 evaluation mode (0, 1, 2, 3, 4)
 *
 * Return value:
 *   u8 frame count (1..64) when intercept/boundary condition triggers.
 *
 * Scratchpad / Actor fields read & written:
 *   Scratch 0xB0..BC   - saved ball state (state, heading, flags, speed, vy, spin)
 *   Scratch 0xF2       (s16) - reference height threshold
 *   Scratch 0xF8/FA/FC (s16) - simulated ball x, y, z position
 *   Scratch 0x290      (u32) - frame counter
 *   Scratch 0x2A9      (u8)  - ball owner (0 = free)
 *   Scratch 0x2F4      (ptr) - ball actor pointer
 *   Ball +0x3BE        (s16) - cleared on entry; set to simulated y on intercept/bounds
 *
 * Magic numbers:
 *   0x30   (48)    = 0.192 m/frame minimum max_speed
 *   0x40   (64)    = max 63 simulated frames (2.1 s at 30 Hz)
 *   0x3320 (13088) = 52.352 m (pitch half-length X bound)
 *   0x2120 (8480)  = 33.920 m (pitch half-width Z bound)
 *   0x6400 (25600) = 160^2 (0.64 m base reach radius squared)
 *   -0x1700 (-5888)= ball y-velocity threshold in mode 3
 *   -0xFF  (-255)  = -1.02 m ball height threshold in mode 1
 *   0x3001 (12289) / 0x6001 (24577) = ball speed thresholds (<< 8)
 *   0x3020 (12320) = 49.28 m (X threshold in mode 2)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern void func_8004BAA4(unsigned int frame_seed);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);

unsigned int func_800484F8(int target_x, int target_z, unsigned int max_speed, unsigned char mode)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
    int tx = (signed short)target_x;
    int tz = (signed short)target_z;
    unsigned int spd = max_speed;
    unsigned int mode_byte = mode & 0xFF;
    int t = 1;
    int best_t = 0;

    if ((spd & 0xFFFF) < 0x30) {
        spd = 0x30;
    }

    H(scratch, 0xF8) = H(ball, 2);
    H(scratch, 0xFA) = H(ball, 4);
    H(scratch, 0xFC) = H(ball, 6);
    B(scratch, 0xB1) = B(ball, 0x3A4);
    B(scratch, 0xB2) = (unsigned char)H(ball, 0x3C8);
    B(scratch, 0xB3) = (unsigned char)H(ball, 0x3CA);
    W(scratch, 0xB4) = W(ball, 0x3A0);
    W(scratch, 0xB8) = W(ball, 0xC);
    B(scratch, 0xB0) = B(ball, 0x396);
    B(scratch, 0xBC) = (unsigned char)H(ball, 0x3BC);
    H(ball, 0x3BE) = 0;

    for (t = 1; t < 0x40; t++) {
        int bx, bz, by, ref_y;
        int abs_bx, abs_bz;
        int dx, dz;
        unsigned int dist_sq, reach, reach_sq;

        func_8004BAA4((unsigned int)(t + W(scratch, 0x290)) & 0xFFFF);

        bx = S(scratch, 0xF8);
        abs_bx = (bx < 0) ? -bx : bx;
        bz = S(scratch, 0xFC);
        abs_bz = (bz < 0) ? -bz : bz;

        if (abs_bx >= 0x3320 || abs_bz >= 0x2120) {
            if (best_t == 0) {
                best_t = t;
            }
            ball = (unsigned char *)W(scratch, 0x2F4);
            if (S(ball, 0x3BE) == 0) {
                H(ball, 0x3BE) = H(scratch, 0xFA);
            }
            if (mode_byte != 0) {
                best_t = t;
            }
            return (unsigned int)best_t & 0xFF;
        }

        dx = tx - bx;
        dz = tz - bz;
        dist_sq = (unsigned int)(dx * dx + dz * dz);

        reach = (spd & 0xFFFF) * (unsigned int)t;
        reach_sq = reach * reach + 0x6400;

        by = S(scratch, 0xFA);
        ref_y = S(scratch, 0xF2);
        if (ref_y < by) {
            if (mode_byte == 3 && by < ref_y + 0x80 && SW(scratch, 0xB8) < -0x1700) {
                continue;
            }
            if (best_t == 0) {
                best_t = t;
            }
            if (reach_sq < dist_sq) {
                continue;
            }

            ball = (unsigned char *)W(scratch, 0x2F4);
            if (S(ball, 0x3BE) == 0) {
                H(ball, 0x3BE) = H(scratch, 0xFA);
            }

            if (mode_byte == 0) {
                return (unsigned int)best_t & 0xFF;
            }
            if (((mode - 3) & 0xFF) < 2) {
                best_t = t;
                return (unsigned int)best_t & 0xFF;
            }
            if (B(scratch, 0x2A9) == 0 && W(scratch, 0xB4) < 0x3001) {
                best_t = t;
                return (unsigned int)best_t & 0xFF;
            }

            if (mode_byte == 1) {
                unsigned int ang;
                unsigned int cw_diff;

                if (S(scratch, 0xFA) < -0xFF) {
                    best_t = t;
                    return (unsigned int)best_t & 0xFF;
                }
                ang = func_8005B36C(tx, tz, S(scratch, 0xF8), S(scratch, 0xFC));
                cw_diff = (B(scratch, 0xB1) - ang) & 0xFF;
                if (cw_diff >= 0x81) {
                    ang = func_8005B36C(tx, tz, S(scratch, 0xF8), S(scratch, 0xFC));
                    if ((int)((ang - B(scratch, 0xB1)) & 0xFF) < 0x41) {
                        best_t = t;
                        return (unsigned int)best_t & 0xFF;
                    }
                } else {
                    ang = func_8005B36C(tx, tz, S(scratch, 0xF8), S(scratch, 0xFC));
                    if ((int)((B(scratch, 0xB1) - ang) & 0xFF) < 0x41) {
                        best_t = t;
                        return (unsigned int)best_t & 0xFF;
                    }
                }
            } else if (mode_byte == 2) {
                unsigned int ang = func_8005B36C(tx, tz, S(scratch, 0xF8), S(scratch, 0xFC));
                unsigned int cw_diff = (B(scratch, 0xB1) - ang) & 0xFF;
                int abs_x;

                if (cw_diff >= 0x81) {
                    ang = func_8005B36C(tx, tz, S(scratch, 0xF8), S(scratch, 0xFC));
                    if ((int)((ang - B(scratch, 0xB1)) & 0xFF) < 0x3D) {
                        best_t = t;
                        return (unsigned int)best_t & 0xFF;
                    }
                } else {
                    ang = func_8005B36C(tx, tz, S(scratch, 0xF8), S(scratch, 0xFC));
                    if ((int)((B(scratch, 0xB1) - ang) & 0xFF) < 0x3D) {
                        best_t = t;
                        return (unsigned int)best_t & 0xFF;
                    }
                }

                abs_x = S(scratch, 0xF8);
                if (abs_x < 0) {
                    abs_x = -abs_x;
                }
                if (abs_x < 0x3020 && W(scratch, 0xB4) < 0x6001) {
                    best_t = t;
                    return (unsigned int)best_t & 0xFF;
                }
            }
        }
    }

    if (mode_byte != 0) {
        best_t = t;
    }
    return (unsigned int)best_t & 0xFF;
}
