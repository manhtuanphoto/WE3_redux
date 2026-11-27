/*
 * func_8008D358
 *
 * Purpose:
 *   Runs the goalkeeper aerial/high-ball punch, parry, or catch-jump substate
 *   machine (actor+0x397 cases 0..16 and default parry/deflect via func_8009462C).
 *   Evaluates reaction delay, jump reach/height against the player's GK/jump stat,
 *   steps toward the intercept point, animates the jump/punch, and deflects the
 *   ball away from goal if reached.
 *
 * Arguments:
 *   actor - pointer to the 1000-byte goalkeeper actor struct.
 *
 * Return value:
 *   None (void).
 *
 * Actor fields read/written:
 *   Reads:  +0x02 (x), +0x06 (z), +0x08 (vx), +0x10 (vz), +0x38D (actor id),
 *           +0x38E (anim id), +0x390 (anim frame), +0x392 (team slot),
 *           +0x397 (substate), +0x398 (side), +0x3A0 (speed), +0x3AA (body heading),
 *           +0x3AB (target heading), +0x3AC (mirror/side), +0x3B9 (reaction stat*10),
 *           +0x3C2/+0x3C4/+0x3C6 (target intercept x/y/z)
 *   Writes: +0x02 (x), +0x06 (z), +0x08 (vx), +0x10 (vz), +0x26 (rotation),
 *           +0x396 (state = 0x82 or 0x8D), +0x397 (substate), +0x3A0 (speed),
 *           +0x3AA (body heading), +0x3AC (mirror/side), +0x3C2/+0x3C4/+0x3C6
 *
 * Scratchpad fields read/written:
 *   Reads:  0x36/0x66 (ball sample x/z), 0x40/0x58/0x70 (ball sample xyz),
 *           0x46/0x76 (ball sample x/z), 0x4E (ball sample y height),
 *           0x2AC (GK/active actor id), 0x2F4 (ball actor ptr),
 *           0x31C (ball timer, frames since touch)
 *   Ball actor (Scratchpad + 0x2F4):
 *     Reads: +0x02 (x), +0x06 (z), +0x0C (vy), +0x3A4 (move heading)
 *
 * Magic numbers / unit conversions (250 raw = 1 m, 30 frames/s):
 *   0xE    = 14 frames (0.467 s base reaction threshold minus stat/10)
 *   -0x80  = -128 raw (0.512 m above ground, high-ball height check)
 *   0x19   = 25 raw (0.100 m height tolerance for jump catch)
 *   0x40   = 64 raw (0.256 m arrive/snap distance; 0x20 = 32 raw max step)
 *   0x2E   = 46 frames (1.533 s end of recovery anim)
 *   736    = ball vy unit divisor (from 0xB21642C9 magic multiply)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_8010C330[];

extern void func_8006EC5C(unsigned char *actor);
extern int func_8005B7CC(int target, int cur, int step);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_800B1128(unsigned int val);
extern int func_8001E694(int angle, int scale);
extern int func_8001E5A4(int angle, int scale);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005BD1C(unsigned char *actor);
extern void func_8005B864(unsigned char *actor, int speed, int arg2);
extern int func_8001E6B8(int n);
extern int func_8005C7A0(int n);
extern int func_8009462C(unsigned char *actor, int arg1, int heading, int arg3, int arg4);
extern void func_80057A14(unsigned char *actor);
extern void func_800954E8(unsigned char *actor);

void func_8008D358(unsigned char *actor)
{
    unsigned int substate = B(actor, 0x397);
    int target_dir;

    if (substate < 0x11U) {
        switch (substate) {
        case 0: {
            int react = 0xE - ((B(actor, 0x3B9) / 10U) & 0xFF);
            unsigned char new_ang;
            int rot;
            int dx;
            int dz;
            unsigned int dist_sq;
            int target_y;
            unsigned int dist;
            int move_ang;

            if (S(Scratchpad, 0x31C) < react && B(actor, 0x38D) != B(Scratchpad, 0x2AC)) {
                func_8006EC5C(actor);
                return;
            }

            new_ang = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
            B(actor, 0x3AA) = new_ang;
            rot = 0x1C0 - new_ang;
            S(actor, 0x26) = (signed short)((rot % 256) << 4);

            dx = S(actor, 0x02) - S(actor, 0x3C2);
            dz = S(actor, 0x06) - S(actor, 0x3C6);
            dist_sq = (unsigned int)(dx * dx + dz * dz);
            target_y = S(actor, 0x3C4);

            if (target_y < -0x80) {
                int off = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
                int reach_stat = (int)((W(D_8010C330, off) >> 11) & 0x7F) + 0x64;
                int reach_scaled = reach_stat << 8;
                int reach_r = (reach_scaled / 180) * reach_scaled;
                unsigned int reach_sq = (unsigned int)(reach_r / 180);

                if (reach_sq < dist_sq) {
                    goto step_to_intercept;
                }
                {
                    int max_jump_y = (-296 * reach_stat) / 180;
                    int diff_y = target_y - max_jump_y;
                    if (diff_y < 0) {
                        diff_y = -diff_y;
                    }
                    if (diff_y >= 0x19) {
                        goto step_to_intercept;
                    }
                }
            }

            func_8005BA30(actor, 0x64, 0);
            B(actor, 0x397) = 8;
            W(actor, 0x3A0) >>= 1;
            return;

        step_to_intercept:
            move_ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(actor, 0x3C2), S(actor, 0x3C6));
            dist = func_800B1128(dist_sq);
            if (dist >= 0x40U) {
                unsigned int step = (dist - 0x40U) / 5U;
                int vx;
                int vz;
                if (step >= 0x20U) {
                    step = 0x20U;
                }
                vx = func_8001E694(move_ang & 0xFF, (int)step);
                vz = func_8001E5A4(move_ang & 0xFF, (int)step);
                SW(actor, 0x08) = vx;
                SW(actor, 0x10) = vz;
                H(actor, 0x02) = (unsigned short)(H(actor, 0x02) + vx);
                H(actor, 0x06) = (unsigned short)(H(actor, 0x06) + vz);
            } else {
                SW(actor, 0x08) = 0;
                SW(actor, 0x10) = 0;
            }
            func_8005BA30(actor, 0x65, 0);
            B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
            return;
        }

        case 1: {
            unsigned char new_ang;
            int rot;

            func_8005BD1C(actor);
            new_ang = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
            B(actor, 0x3AA) = new_ang;
            rot = 0x1C0 - new_ang;
            S(actor, 0x26) = (signed short)((rot % 256) << 4);

            H(actor, 0x02) = (unsigned short)(H(actor, 0x02) + SW(actor, 0x08));
            H(actor, 0x06) = (unsigned short)(H(actor, 0x06) + SW(actor, 0x10));

            if (B(actor, 0x390) >= 5) {
                SW(actor, 0x08) = (SW(actor, 0x08) * 9) / 10;
                SW(actor, 0x10) = (SW(actor, 0x10) * 9) / 10;
            }
            if (B(actor, 0x390) == 0) {
                func_8005BA30(actor, 0x62, 0x17);
                B(actor, 0x396) = 0x8D;
                B(actor, 0x397) = 1;
            }
            return;
        }

        case 2: {
            unsigned char new_ang;
            int rot;

            func_8005BD1C(actor);
            new_ang = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
            B(actor, 0x3AA) = new_ang;
            rot = 0x1C0 - new_ang;
            S(actor, 0x26) = (signed short)((rot % 256) << 4);
            if (B(actor, 0x390) == 0x2E) {
                func_8005BA30(actor, 1, 0);
                B(actor, 0x396) = 0x82;
                B(actor, 0x397) = 0;
            }
            return;
        }

        case 8: {
            unsigned char new_ang;
            int rot;

            func_8005BD1C(actor);
            new_ang = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
            B(actor, 0x3AA) = new_ang;
            rot = 0x1C0 - new_ang;
            S(actor, 0x26) = (signed short)((rot % 256) << 4);
            func_8005B864(actor, H(actor, 0x3A0), rot);
            W(actor, 0x3A0) = (W(actor, 0x3A0) << 3) / 10U;
            if (B(actor, 0x390) == 0) {
                func_8005BA30(actor, 0x2C, 0x23);
                B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
            }
            return;
        }

        case 9:
            func_8005BD1C(actor);
            if (B(actor, 0x390) == 0x2E) {
                func_8005BA30(actor, 1, 0);
                B(actor, 0x396) = 0x82;
                B(actor, 0x397) = 0;
            }
            return;

        case 16: {
            int ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(Scratchpad, 0x46), S(Scratchpad, 0x76));
            unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
            unsigned char ball_dir = B(ball, 0x3A4);
            int d = (ang - ball_dir) & 0xFF;
            int in_cone = (d < 0x81) ? (d < 0x40) : (((ball_dir - ang) & 0xFF) < 0x40);

            if (in_cone) {
                int b2p = func_8005B36C(S(ball, 0x02), S(ball, 0x06), S(actor, 0x02), S(actor, 0x06));
                B(actor, 0x3AC) = (unsigned char)(((b2p - ang) & 0xFF) < 0x81U);
                H(actor, 0x3C2) = H(Scratchpad, 0x40);
                H(actor, 0x3C4) = H(Scratchpad, 0x58);
                H(actor, 0x3C6) = H(Scratchpad, 0x70);
                B(actor, 0x397) = 0;
            }
            return;
        }

        default:
            break;
        }
    }

    if (H(actor, 0x38E) == 0x64) {
        int off = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        int reach_stat = (int)((W(D_8010C330, off) >> 11) & 0x7F) + 0x64;
        int min_y = (-160 * reach_stat) / 180;

        if (S(Scratchpad, 0x4E) < min_y) {
            int delta = func_8001E6B8(0x20) + 0x48;
            target_dir = B(actor, 0x3AC) ? (B(actor, 0x3AA) + delta) : (B(actor, 0x3AA) - delta);
        } else {
            target_dir = B(actor, 0x3AA) + func_8005C7A0(0x10);
        }
    } else {
        int ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(Scratchpad, 0x36), S(Scratchpad, 0x66));
        int side_sign = (((ang - B(actor, 0x3AA)) & 0xFF) < 0x81U) ? -1 : 1;
        int off = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        int reach_stat = (int)((W(D_8010C330, off) >> 11) & 0x7F) + 0x64;
        int min_y = (-160 * reach_stat) / 180;
        int delta;
        unsigned char *ball;

        if (S(Scratchpad, 0x4E) < min_y) {
            delta = func_8001E6B8(0x14) + 0x34;
        } else {
            delta = func_8001E6B8(0x20) + 0x30;
        }
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        target_dir = B(ball, 0x3A4) + side_sign * delta;
    }

    {
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        int abs_vy = SW(ball, 0x0C) / 736;
        int r1;
        int r2;

        if (abs_vy < 0) {
            abs_vy = -abs_vy;
        }
        /* UNCLEAR: abs_vy is left in $a0 before func_8001E6B8(4) overwrites $a0 in the
         * delay slot; the second func_8001E6B8 call relies on $a0 surviving or holding 4. */
        r1 = func_8001E6B8(4);
        r2 = func_8001E6B8(abs_vy);
        if ((func_8009462C(actor, 0xC, target_dir & 0xFF, 0, r2 - r1) & 0xFF) != 0) {
            func_80057A14(actor);
        }
        func_800954E8(actor);
    }
}
