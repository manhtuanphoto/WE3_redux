/*
 * func_8002C28C
 *
 * Purpose:
 *   Selects a pass or cross target for a ball-carrying player, refines the pass
 *   lead via func_8002CF5C and func_8002ECB0, quantizes the relative pass angle
 *   into 16 sectors around the player's body/ball orientation, and initiates the
 *   appropriate kicking/turning/backheel animation and substate transition.
 *
 * Arguments:
 *   actor - pointer to the 1000-byte player actor struct.
 *
 * Return value:
 *   None (void).
 *
 * Actor fields read/written:
 *   Reads:  +0x02 (x), +0x06 (z), +0x38D (actor id), +0x38E (current anim id),
 *           +0x392 (team slot 0..10), +0x396 (state), +0x398 (side 0/1),
 *           +0x399 (attack dir), +0x39A, +0x3A0 (speed), +0x3AA (body heading),
 *           +0x3C2 (target actor id), +0x3C4 (pass mode), +0x3C6 (lead x offset)
 *   Writes: +0x396 (state = 0 or 0x1B when target_id==0 && +0x39A!=0),
 *           +0x397 (substate = 0, 2, 3, 5, or 6), +0x3A0 (speed -= 0x12, min 0),
 *           +0x3AA (body heading), +0x3AB (target heading), +0x3AC (mirror/foot),
 *           +0x3C2 (target actor id), +0x3C4 (pass mode), +0x3C6 (lead x offset),
 *           +0x3CA (pass distance), +0x3CC (pass heading), +0x3CE (16-sector angle),
 *           +0x3D1 (= 2 when kick check fails)
 *   Target actor (D_800FF748 + (target_id % 24) * 1000):
 *     Reads: +0x02 (x), +0x06 (z), +0x08 (vx), +0x10 (vz)
 *   Ball actor (Scratchpad + 0x2F4):
 *     Reads:  +0x02 (x), +0x06 (z)
 *     Writes: +0x396 (= 3 on sector 3..6/10..13 pass)
 *
 * Scratchpad fields read/written:
 *   Reads:  0x3C / 0x6C (ball reference x/z), 0x28C (pad dir), 0x29A (match phase),
 *           0x2A8 (last-touch actor id), 0x2F4 (ball actor ptr)
 *   Writes: 0xFE / 0x100 (cross target x/z when target_id == 0),
 *           0x2A8 (last-touch actor id)
 *
 * Magic numbers / unit conversions (250 raw = 1 m, 30 frames/s):
 *   0x200  = 512 raw (2.048 m, normal pass lead margin)
 *   0x600  = 1536 raw (6.144 m, through-pass lead margin)
 *   0x8B6  = 2230 raw (8.920 m, cross target |z| offset)
 *   0xC01  = 3073 raw (12.292 m, pass distance threshold to clear lead)
 *   0xE00  = 3584 raw (14.336 m, short-pass threshold for fancy/backheel pass)
 *   0x1000 = 4096 raw (16.384 m, default pass distance in mode 6)
 *   0x21E3 = 8675 raw (34.700 m, forward x threshold for auto cross into box)
 *   0x28F0 = 10480 raw (41.920 m, minimum cross target |x|)
 *   0x400  = 1024 raw (4.096 m, random x range on cross target)
 *   0x100 / 0x80 = 256 / 128 raw (1.024 / 0.512 m, cross x pullback)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_800C6B74[];
extern unsigned char D_8010C32E[];
extern unsigned char D_8010C330[];

extern int func_8002CA7C(unsigned char *actor, unsigned int angle);
extern void func_8002CF5C(unsigned char *actor);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_800B1128(unsigned int val);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern int func_8001E6B8(int n);
extern int func_8005C7A0(int n);
extern int func_8002ECB0(unsigned char *actor, int heading, unsigned int dist);
extern int func_8005B7CC(int target, int cur, int step);
extern void func_80049DF0(unsigned char *actor, int heading, int arg2, int arg3);
extern int func_8005385C(unsigned char *actor, int arg1);

void func_8002C28C(unsigned char *actor)
{
    int pass_angle;
    unsigned int pass_dist;
    int pred_x;
    int pred_z;
    int target_id;
    int pass_mode;
    int ball_ang;
    int ref_ang;
    int diff;
    int sector;

    if (B(actor, 0x397) != 2 && B(Scratchpad, 0x29A) == 1) {
        if (S(actor, 0x3C4) != 4) {
            unsigned char pad_dir = B(Scratchpad, 0x28C);
            unsigned char dir_ang;
            if (pad_dir != 0) {
                dir_ang = D_800C6B74[pad_dir];
            } else {
                dir_ang = B(actor, 0x3AA);
            }
            H(actor, 0x3C2) = (unsigned short)(func_8002CA7C(actor, dir_ang & 0xFF) & 0xFF);
        }
        func_8002CF5C(actor);
    }

    target_id = S(actor, 0x3C2);
    if (target_id != 0) {
        int target_idx = (signed short)(target_id % 24);
        unsigned char *target = D_800FF748 + target_idx * 1000;
        int lead_x = S(actor, 0x3C6);
        int base_x = S(target, 0x02) + (SW(target, 0x08) << 2);
        int raw_x = (B(actor, 0x399) != 0) ? (base_x - lead_x) : (base_x + lead_x);
        unsigned char *ball;
        int dx;
        int dz;

        pred_x = (signed short)raw_x;
        pred_z = (signed short)(H(target, 0x06) + (SW(target, 0x10) << 2));
        pass_angle = func_8005B36C(S(actor, 0x02), S(actor, 0x06), pred_x, pred_z);

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        dx = S(ball, 0x02) - pred_x;
        dz = S(ball, 0x06) - pred_z;
        pass_dist = func_800B1128((unsigned int)(dx * dx + dz * dz));

        pass_mode = S(actor, 0x3C4);
        if (pass_mode == 1 || pass_mode == 4) {
            unsigned char body_ang = B(actor, 0x3AA);
            int d = (pass_angle - body_ang) & 0xFF;
            int within_cone = (d < 0x81) ? (d < 0x60) : (((body_ang - pass_angle) & 0xFF) < 0x60);

            if (!within_cone) {
                S(actor, 0x3C4) = 0;
                if (pass_dist >= 0xC01U) {
                    S(actor, 0x3C6) = 0;
                }
            }
            pass_mode = S(actor, 0x3C4);
        }
        pass_angle &= 0xFF;
    } else {
        int cross_z;
        int cross_x;
        int rand_off;
        int min_x;
        int atk_dir;
        int fwd_x;
        int goal_dir;
        int d;
        int in_cone;
        int dx;
        int dz;

        if (B(actor, 0x39A) != 0) {
            if (B(actor, 0x38D) == B(Scratchpad, 0x2A8)) {
                func_8005BA30(actor, 1, 0);
                B(actor, 0x396) = 0;
                B(actor, 0x397) = 0;
            } else {
                B(actor, 0x396) = 0x1B;
                B(actor, 0x397) = 0;
                S(actor, 0x3C2) = 0xFF;
            }
            return;
        }

        fwd_x = (B(actor, 0x399) != 0) ? -S(actor, 0x02) : S(actor, 0x02);
        if (fwd_x < 0x21E3) {
            S(actor, 0x3C4) = 6;
            pass_dist = 0x1000;
            pass_mode = 6;
            pass_angle &= 0xFF;
        } else {
            atk_dir = B(actor, 0x399);
            goal_dir = atk_dir << 7;
            d = (goal_dir - B(actor, 0x3AA)) & 0xFF;
            cross_z = 0x8B6;
            in_cone = (d < 0x81) ? (d < 0x31) : (((B(actor, 0x3AA) - goal_dir) & 0xFF) < 0x31);

            if (!in_cone) {
                S(actor, 0x3C4) = 6;
                pass_dist = 0x1000;
                pass_mode = 6;
                pass_angle &= 0xFF;
            } else {
                S(actor, 0x3C4) = 5;
                if (S(actor, 0x06) > 0) {
                    cross_z = -0x8B6;
                }
                rand_off = func_8001E6B8(0x100) + 0x80;
                cross_x = (B(actor, 0x399) != 0) ? (S(actor, 0x02) + rand_off) : (S(actor, 0x02) - rand_off);

                min_x = (signed short)(func_8001E6B8(0x400) + 0x28F0);
                S(Scratchpad, 0xFE) = (signed short)min_x;

                if (B(actor, 0x399) != 0) {
                    if (-(signed short)cross_x < min_x) {
                        cross_x = -S(Scratchpad, 0xFE);
                    }
                } else {
                    if ((signed short)cross_x < min_x) {
                        cross_x = S(Scratchpad, 0xFE);
                    }
                }

                pred_x = (signed short)cross_x;
                pred_z = (signed short)cross_z;
                pass_angle = func_8005B36C(S(actor, 0x02), S(actor, 0x06), pred_x, pred_z);
                pass_angle += func_8005C7A0(2);

                dx = S(actor, 0x02) - pred_x;
                dz = S(actor, 0x06) - pred_z;
                pass_dist = func_800B1128((unsigned int)(dx * dx + dz * dz));

                S(Scratchpad, 0xFE) = (signed short)cross_x;
                S(Scratchpad, 0x100) = (signed short)cross_z;
                pass_mode = S(actor, 0x3C4);
                pass_angle &= 0xFF;
            }
        }
    }

    S(actor, 0x3CC) = (signed short)pass_angle;
    S(actor, 0x3CA) = (signed short)pass_dist;

    switch ((unsigned int)pass_mode) {
    case 0:
    case 2:
    case 3:
        pass_angle = func_8002ECB0(actor, B(actor, 0x3CC), pass_dist + 0x200);
        break;
    case 1:
    case 4:
        pass_angle = func_8002ECB0(actor, B(actor, 0x3CC), pass_dist + 0x600);
        break;
    default:
        pass_angle = B(actor, 0x3CC);
        break;
    }

    ball_ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(Scratchpad, 0x3C), S(Scratchpad, 0x6C));
    diff = (ball_ang - B(actor, 0x3AA)) & 0xFF;
    if (diff < 0x81) {
        if (diff < 0x40) {
            ref_ang = ball_ang;
        } else {
            ref_ang = B(actor, 0x3AA);
        }
    } else {
        if (((B(actor, 0x3AA) - ball_ang) & 0xFF) < 0x40) {
            ref_ang = ball_ang;
        } else {
            ref_ang = B(actor, 0x3AA);
        }
    }

    sector = (unsigned int)(((pass_angle - ref_ang) + 8) & 0xFF) >> 4;
    S(actor, 0x3CE) = (signed short)sector;

    switch (sector & 0xFF) {
    case 0: {
        int off = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        B(actor, 0x3AC) = (unsigned char)((W(D_8010C330, off) >> 4) & 1);
        func_8005BA30(actor, 0x10, 6);
        func_80049DF0(actor, pass_angle & 0xFF, 9, 3);
        B(actor, 0x397) = 3;
        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(pass_angle & 0xFF, B(actor, 0x3AA), 0xC);
        B(actor, 0x3AB) = (unsigned char)pass_angle;
        break;
    }

    case 1:
    case 2:
    case 14:
    case 15:
        B(actor, 0x3AC) = (unsigned char)((unsigned int)sector >> 3);
        func_8005BA30(actor, 0x10, 6);
        pass_angle = func_8005B7CC(pass_angle & 0xFF, ref_ang & 0xFF, 0xC);
        func_80049DF0(actor, pass_angle & 0xFF, 9, 3);
        B(actor, 0x397) = 3;
        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(pass_angle & 0xFF, B(actor, 0x3AA), 0xC);
        B(actor, 0x3AB) = (unsigned char)pass_angle;
        break;

    case 3:
    case 4:
    case 5:
    case 6:
    case 10:
    case 11:
    case 12:
    case 13:
        if (pass_dist < 0xE00U) {
            int off = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
            int tech = H(D_8010C32E, off) & 0xF;
            if (9 - tech < 3) {
                if (func_8005385C(actor, 1) == 0) {
                    if (B(actor, 0x396) != 0x18) {
                        B(actor, 0x3D1) = 2;
                        B(actor, 0x397) = 2;
                    }
                    return;
                }
                func_8005BA30(actor, 0x12, 1);
                B(actor, 0x3AC) = (unsigned char)(((unsigned int)sector >> 3) ^ 1);
                if (B(actor, 0x3AC) != 0) {
                    pass_angle = (pass_angle & 0xFF) - 0x28;
                } else {
                    pass_angle = (pass_angle & 0xFF) + 0x28;
                }
                pass_angle = func_8005B7CC(pass_angle & 0xFF, ref_ang & 0xFF, 0xC);
                func_80049DF0(actor, ref_ang & 0xFF, 4, 3);
                B(actor, 0x397) = 5;
                B(actor, 0x3AA) = (unsigned char)func_8005B7CC(pass_angle & 0xFF, B(actor, 0x3AA), 0xC);
                B(actor, 0x3AB) = (unsigned char)pass_angle;
                return;
            }
        }
        if (func_8005385C(actor, 1) == 0) {
            if (B(actor, 0x396) != 0x18) {
                B(actor, 0x3D1) = 2;
                B(actor, 0x397) = 2;
            }
            return;
        } else {
            unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
            unsigned int spd;

            B(Scratchpad, 0x2A8) = B(actor, 0x38D);
            B(ball, 0x396) = 3;
            B(actor, 0x3AC) = (unsigned char)((unsigned int)sector >> 3);
            func_8005BA30(actor, 0x10, 4);
            pass_angle = func_8005B7CC(ref_ang & 0xFF, pass_angle & 0xFF, 0x10);
            B(actor, 0x3AB) = (unsigned char)pass_angle;
            B(actor, 0x3AA) = (unsigned char)func_8005B7CC(pass_angle & 0xFF, B(actor, 0x3AA), 0x18);
            spd = W(actor, 0x3A0);
            W(actor, 0x3A0) = (spd < 0x13U) ? 0 : (spd - 0x12);
            B(actor, 0x397) = 3;
        }
        break;

    case 7:
    case 8:
    case 9:
    default:
        if (func_8005385C(actor, 1) == 0) {
            if (B(actor, 0x396) != 0x18) {
                B(actor, 0x3D1) = 2;
                B(actor, 0x397) = 2;
            }
        } else {
            int off = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
            B(actor, 0x3AC) = (unsigned char)((W(D_8010C330, off) >> 4) & 1);
            if (H(actor, 0x38E) != 0xC) {
                func_8005BA30(actor, 0xC, 0);
            }
            B(actor, 0x3AB) = (unsigned char)pass_angle;
            B(actor, 0x3AA) = (unsigned char)func_8005B7CC((pass_angle + 0x80) & 0xFF, B(actor, 0x3AA), 0xC);
            B(actor, 0x397) = 6;
        }
        break;
    }
}
