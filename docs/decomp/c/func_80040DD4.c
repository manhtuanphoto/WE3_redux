/*
 * func_80040DD4 - Sliding tackle / lunge ball-contact and foul resolution.
 *
 * Purpose:
 *   Evaluates whether a tackling actor contacts the ball (clean win, clearance, or deflection)
 *   and whether the tackle collides with or fouls the target opponent actor.
 *
 * Arguments:
 *   actor (u8*): tackling player actor struct (1000 bytes).
 *   mode  (int): tackle contact mode (0 = kick/clear, 1..2 = trap/control, >=3 = contact disabled).
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F800036/0x4E/0x66 (s16, R): ball predicted xyz at frame 0 (0x3350 = 52.54 m goal line, 0x3C0 = 3.84 m post).
 *   - 0x1F8000F0/0xF2/0xF4 (s16, R): actor/tackle contact reference xyz.
 *   - 0x1F80029A (u8, R): match phase (1 = open play).
 *   - 0x1F8002A8 (u8, W): last-touch actor ID.
 *   - 0x1F8002A9 (u8, R): current ball owner (0 = free).
 *   - 0x1F8002AA (u8, R): previous ball owner.
 *   - 0x1F8002AD (u8, W): foul offender actor ID.
 *   - 0x1F8002AE (u8, W): fouled target actor ID.
 *   - 0x1F8002AF (u8, R/W): foul called flag.
 *   - 0x1F8002B0 (u32, R/W): ball event code (4 = shot/clearance, 0xA = tackle touch).
 *   - 0x1F8002CF[2] (u8, R/W): team possession flags.
 *   - 0x1F8002F4 (u8*, R): pointer to ball actor.
 *   - 0x1F8002FC (u8, R): early-exit freeze/replay flag.
 *   - 0x1F80031C (s16, R/W): ball timer (frames since last touch; 0xF = 0.50s, 0x10 = 0.53s).
 *
 * Actor fields reads/writes:
 *   - actor: +0x02/+0x06 (s16 x/z), +0x38D (u8 id), +0x38E (u8 anim), +0x390 (u8 frame),
 *            +0x392 (u8 team slot), +0x398 (u8 side), +0x399 (u8 attack dir), +0x3A0 (u32 speed),
 *            +0x3AA (u8 body heading), +0x3AC (u8 mirror), +0x3C2 (s16 target actor, R/W),
 *            +0x3C4 (u16, W=0), +0x3C8 (s16 contact window, R/W), +0x3CA (s16, R), +0x3CC (s16, R).
 *   - target: +0x02/+0x06 (s16 x/z), +0x38D (u8 id), +0x396 (u8 state, W), +0x397 (u8 substate, W),
 *             +0x3D1 (u8, W=0).
 *   - ball:   +0x02/+0x04/+0x06 (s16 xyz, R/W), +0x0C (s32 vy, R), +0x396 (u8 state, W),
 *             +0x3A4 (u8 move heading, R), +0x3B1 (u8, W=2).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x3350 (13136) = 52.54 m (goal line X), 0x33A0 (13216) = 52.86 m (goal target X)
 *   - 0x3C0 (960) = 3.84 m (goalpost Z), 0x80 (128) = 0.51 m tolerance
 *   - 0x4000 (128^2) -> 0.51 m XZ contact radius; -0x9F (-159) = 0.64 m ball height limit
 *   - 0x1FE3 (8163) = 32.65 m X, 0x16F4 (5876) = 23.50 m Z (shot/clearance zone)
 *   - 0x2400 (96^2) -> 0.38 m body-contact radius
 *   - 0x23E3 (9187) = 36.75 m X, 0x12F4 (4852) = 19.41 m Z (penalty area)
 *   - 0x143FF (~288^2) -> 1.15 m, 0xFFFF (~256^2) -> 1.02 m
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];

extern void func_8005C2AC(unsigned char *actor, int anim, int frame, int heading, int mirror);
extern void func_80056240(unsigned char *actor);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8005C7A0(int n);
extern int func_8005B7CC(int target, int cur, int step);
extern int func_8001E6B8(int n);
extern void func_8004B0F0(unsigned char *actor, int heading, int power, int lift);
extern int func_8001E694(int angle, int scale);
extern void func_80089D00(unsigned char *actor, int a1, int x, int z);
extern void func_800512B8(unsigned char *actor, int a1);
extern int func_8009462C(unsigned char *actor, int a1, int heading, int a3, int lift);
extern int func_80052D90(unsigned char *actor);
extern int func_80052720(unsigned char *actor, unsigned char *target);
extern void func_800492FC(unsigned char *actor);
extern void func_800171C4(int sfx);
extern void func_80057370(void);

void func_80040DD4(unsigned char *actor, int mode)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *ball;
    int ball_touched = 0;
    int abs_x, abs_z, diff_post, abs_ball_z;
    int target_id;

    if (B(scratch, 0x2FC) != 0) {
        return;
    }

    abs_x = S(scratch, 0x36);
    if (abs_x < 0) abs_x = -abs_x;
    if (abs_x >= 0x3350 && B(scratch, 0x29A) == 1) {
        abs_z = S(scratch, 0x66);
        if (abs_z < 0) abs_z = -abs_z;
        diff_post = abs_z - 0x3C0;
        if (diff_post < 0) diff_post = -diff_post;
        if (diff_post < 0x81) {
            ball = (unsigned char *)W(scratch, 0x2F4);
            abs_ball_z = S(ball, 6);
            if (abs_ball_z < 0) abs_ball_z = -abs_ball_z;
            if (abs_z < abs_ball_z) {
                return;
            }
        }
    }

    func_8005C2AC(actor, B(actor, 0x38E), B(actor, 0x390), B(actor, 0x3AA), B(actor, 0x3AC));

    if (S(actor, 0x3C8) != 0) {
        int dx = S(scratch, 0xF0) - S(scratch, 0x36);
        int dz = S(scratch, 0xF4) - S(scratch, 0x66);
        int dist_sq = dx * dx + dz * dz;

        if (dist_sq < 0x4000 && S(scratch, 0x4E) >= -0x9F) {
            int m;
            func_80056240(actor);
            H(actor, 0x3C4) = 0;

            if (B(scratch, 0x2A9) == 0 && S(scratch, 0x31C) < 0xF &&
                B(actor, 0x38D) != B(scratch, 0x2AA)) {
                mode = 0;
            }

            m = mode & 0xFF;
            if (m == 0) {
                int fwd_x, abs_z2;
                S(actor, 0x3C8) = 0;
                ball = (unsigned char *)W(scratch, 0x2F4);
                S(ball, 2) = (S(scratch, 0x36) + S(scratch, 0xF0)) >> 1;
                S(ball, 4) = (S(scratch, 0x4E) + S(scratch, 0xF2)) >> 1;
                S(ball, 6) = (S(scratch, 0x66) + S(scratch, 0xF4)) >> 1;

                fwd_x = (B(actor, 0x399) != 0) ? -S(actor, 2) : S(actor, 2);
                abs_z2 = S(actor, 6);
                if (abs_z2 < 0) abs_z2 = -abs_z2;

                if (B(scratch, 0x2A9) == 0 && S(scratch, 0x31C) >= 0x10 &&
                    fwd_x >= 0x1FE3 && abs_z2 < 0x16F4) {
                    int goal_x = (B(actor, 0x399) != 0) ? -0x33A0 : 0x33A0;
                    int ang = func_8005B36C(S(ball, 2), S(ball, 6), goal_x, 0);
                    int heading = func_8005B7CC((ang + func_8005C7A0(0x10)) & 0xFF, B(actor, 0x3AA), 0x18);
                    int power = S(actor, 0x3CA) + func_8001E6B8(8) + 0x60;
                    int lift = func_8001E6B8(4) + 3;
                    unsigned char side = B(actor, 0x398);

                    func_8004B0F0(actor, heading & 0xFF, power, lift);
                    B(D_800FF748, 0x8EC8 + side)++;
                    B(D_800FF748, 0x88D1 + side * 22 + B(actor, 0x392))++;
                    W(scratch, 0x2B0) = 4;
                    B(D_800FF748, 0xAC4F) = 0xFF;
                    B(D_800FF748, 0xAC4E) = B(actor, 0x38D);
                } else {
                    int heading, power, lift;
                    int off_x, off_z;

                    if (B(scratch, 0x2A9) != 0 && B(scratch, 0x2CF + B(actor, 0x398)) == 1) {
                        S(actor, 0x3C2) = B(scratch, 0x2A9);
                    }
                    heading = (B(actor, 0x3AA) + func_8005C7A0(0x10)) & 0xFF;
                    if (S(actor, 0x3C2) != 0) {
                        power = func_8001E6B8(4) + 0x48;
                        lift = 0;
                    } else {
                        power = (int)W(actor, 0x3A0) + func_8001E6B8(4) + 8;
                        if (power < 0x48) {
                            power = 0x48;
                        }
                        lift = func_8001E6B8(3);
                    }
                    func_8004B0F0(actor, heading, power, lift);

                    /* Both offsets call func_8001E694 (cos) in original binary */
                    off_x = func_8001E694(heading, 0x200);
                    ball = (unsigned char *)W(scratch, 0x2F4);
                    off_x = (signed short)(S(ball, 2) + off_x);
                    off_z = func_8001E694(heading, 0x200);
                    ball = (unsigned char *)W(scratch, 0x2F4);
                    off_z = (signed short)(S(ball, 6) + off_z);
                    func_80089D00(actor, 0, off_x, off_z);

                    W(scratch, 0x2B0) = 0xA;
                    B(scratch, 0x2CF + B(actor, 0x398)) = 1;
                }

                ball = (unsigned char *)W(scratch, 0x2F4);
                B(ball, 0x396) = 4;
                S(scratch, 0x31C) = (B(actor, 0x399) ^ 1) & 0xFF;
                ball_touched = 1;
            } else if (m < 3) {
                if (S(actor, 0x3C2) == 0 && B(scratch, 0x2A9) != 0 &&
                    B(scratch, 0x2CF + B(actor, 0x398)) == 1) {
                    S(actor, 0x3C2) = B(scratch, 0x2A9);
                }
                B(scratch, 0x2A8) = B(actor, 0x38D);
                ball = (unsigned char *)W(scratch, 0x2F4);
                B(ball, 0x396) = 3;
                ball = (unsigned char *)W(scratch, 0x2F4);
                B(ball, 0x3B1) = 2;
                func_800512B8(actor, 1);
                S(actor, 0x3C8) = 0;
                ball_touched = 1;
            } else {
                ball_touched = 1;
            }
        } else {
            /* Glancing / aerial ball deflection */
            unsigned char body_h = B(actor, 0x3AA);
            unsigned char ball_h;
            unsigned int diff, step;
            int heading, power_adj, abs_vy, lift_r, lift_jit;

            ball = (unsigned char *)W(scratch, 0x2F4);
            ball_h = B(ball, 0x3A4);
            diff = (ball_h - body_h) & 0xFF;
            if (diff < 0x81) {
                step = diff >> 1;
            } else {
                step = ((body_h - ball_h) & 0xFF) >> 1;
            }
            heading = func_8005B7CC(body_h, ball_h, step);
            heading += func_8005C7A0(0x10);

            power_adj = (W(scratch, 0x2B0) == 4) ? (func_8001E6B8(0x14) - 0x18) : 0;

            ball = (unsigned char *)W(scratch, 0x2F4);
            abs_vy = SW(ball, 0xC);
            if (abs_vy < 0) abs_vy = -abs_vy;
            lift_r = func_8001E6B8(abs_vy / 736);
            lift_jit = func_8005C7A0(8);

            if ((func_8009462C(actor, 0xC, heading & 0xFF, power_adj, (signed short)lift_r - lift_jit) & 0xFF) != 0) {
                int h = heading & 0xFF;
                int off_x, off_z;

                /* Both offsets call func_8001E694 (cos) in original binary */
                off_x = func_8001E694(h, 0x200);
                ball = (unsigned char *)W(scratch, 0x2F4);
                off_x = (signed short)(S(ball, 2) + off_x);
                ball_touched = 1;
                off_z = func_8001E694(h, 0x200);
                ball = (unsigned char *)W(scratch, 0x2F4);
                off_z = (signed short)(S(ball, 6) + off_z);
                func_80089D00(actor, 0, off_x, off_z);

                W(scratch, 0x2B0) = 0xA;
                B(scratch, 0x2CF + B(actor, 0x398)) = 1;
                func_80056240(actor);
                H(actor, 0x3C4) = 0;
            }
        }
    }

    target_id = S(actor, 0x3C2);
    if (target_id == 0 || B(scratch, 0x29A) != 1) {
        return;
    }

    {
        unsigned char *target = D_800FF748 + (signed short)(target_id % 24) * 1000;
        int dx_ref, dz_ref, dx_act, dz_act;
        int tx, tz;

        if ((func_80052D90(target) & 0xFF) != 0) {
            return;
        }

        tx = S(target, 2);
        tz = S(target, 6);
        dx_ref = tx - S(scratch, 0xF0);
        dz_ref = tz - S(scratch, 0xF4);
        dx_act = S(actor, 2) - tx;
        dz_act = S(actor, 6) - tz;

        if ((mode & 0xFF) != 1 &&
            ((dx_ref * dx_ref + dz_ref * dz_ref) < 0x2400 ||
             (dx_act * dx_act + dz_act * dz_act) < 0x2400)) {
            int ang, diff;

            S(actor, 0x3C2) = 0;
            B(target, 0x3D1) = 0;
            ang = func_8005B36C(S(actor, 2), S(actor, 6), S(target, 2), S(target, 6));
            diff = (ang - B(actor, 0x3AA)) & 0xFF;
            if (diff < 0x81) {
                ang = func_8005B36C(S(actor, 2), S(actor, 6), S(target, 2), S(target, 6));
                if (((ang - B(actor, 0x3AA)) & 0xFF) >= 0x40) {
                    return;
                }
            } else {
                ang = func_8005B36C(S(actor, 2), S(actor, 6), S(target, 2), S(target, 6));
                if (((B(actor, 0x3AA) - ang) & 0xFF) >= 0x40) {
                    return;
                }
            }

            if (S(actor, 0x3CC) != 0) {
                B(scratch, 0x2AF) = (unsigned char)func_80052720(actor, target);
            } else {
                B(scratch, 0x2AF) = 1;
            }

            if (B(scratch, 0x2AF) != 0) {
                B(scratch, 0x2AD) = B(actor, 0x38D);
                B(scratch, 0x2AE) = B(target, 0x38D);
                func_800492FC(target);
                func_800171C4(6);
            } else {
                B(target, 0x3D1) = 0;
                if (func_8001E6B8(3) != 0) {
                    B(target, 0x396) = 0x10;
                    B(target, 0x397) = 0;
                } else {
                    func_800492FC(target);
                }
                func_80057370();
            }
        } else if ((ball_touched & 0xFF) == 1) {
            int fwd_x = (B(actor, 0x399) != 0) ? -S(actor, 2) : S(actor, 2);
            int abs_az = S(actor, 6);
            if (abs_az < 0) abs_az = -abs_az;

            if (fwd_x >= 0x23E3 && abs_az < 0x12F4) {
                /* Inside penalty area: call foul */
                B(scratch, 0x2AD) = B(actor, 0x38D);
                B(scratch, 0x2AE) = B(target, 0x38D);
                func_800492FC(target);
                func_800171C4(6);
                func_800492FC(target);
                B(scratch, 0x2AF) = 1;
            } else {
                int bx, bz, dx_b, dz_b;
                ball = (unsigned char *)W(scratch, 0x2F4);
                bx = S(ball, 2);
                bz = S(ball, 6);
                dx_b = tx - bx;
                dz_b = tz - bz;

                if ((dx_b * dx_b + dz_b * dz_b) > 0x143FF &&
                    (dx_act * dx_act + dz_act * dz_act) > 0xFFFF) {
                    B(target, 0x396) = 0x18;
                    B(target, 0x397) = 0;
                    B(target, 0x3D1) = 0;
                } else if (func_8001E6B8(8) != 0) {
                    B(target, 0x396) = 0x10;
                    B(target, 0x397) = 0;
                    B(target, 0x3D1) = 0;
                } else {
                    func_800492FC(target);
                    B(target, 0x3D1) = 0;
                }
            }
        }
    }
}
