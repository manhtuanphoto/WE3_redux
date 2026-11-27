/*
 * func_800420E4 - Player pressing / ball-chasing AI state machine.
 *
 * Purpose:
 *   Controls a pressing/chasing player actor (substates 0..4): verifies open-play
 *   and nearest-chaser eligibility, computes pursuit target coordinates (leading
 *   the ball carrier or intercepting a free ball / blocking a carrier), steers/sprints
 *   toward the target, transitions into close-range deceleration or lunge/tackle
 *   prep, and updates actor animation/movement via func_80042BE0 and func_8005B908.
 *
 * Arguments:
 *   actor (unsigned char*): pressing/chasing player actor struct (1000 bytes).
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F800036 / 0x1F800066 (s16, R): current/frame-0 ball X / Z.
 *   - 0x1F8000F8 / 0x1F8000FC (s16, R/W): predicted ball X / Z target.
 *   - 0x1F80029A (u8, R): match phase (1 = open play).
 *   - 0x1F8002A9 (u8, R): ball owner actor ID (0 = free).
 *   - 0x1F8002F4 (u8*, R): pointer to ball actor.
 *   - 0x1F8002FC (u8, R): early-exit freeze/replay flag.
 *   - 0x1F80031C (s16, R): ball timer (frames since last touch).
 *   - 0x1F80031E (u8[2][2], R): nearest-to-ball actor ID per attack direction.
 *   - 0x1F800328 (u8, R): difficulty / pressing level index into D_800C6D64.
 *
 * Actor fields reads/writes:
 *   - actor: +0x02/+0x06 (s16 x/z, R), +0x38D (u8 id, R), +0x38E (u16 anim id, R),
 *            +0x396 (u8 state, W), +0x397 (u8 substate, R/W), +0x399 (u8 attack dir, R),
 *            +0x3A0 (u32 speed, R/W), +0x3A4 (u8 move heading, R/W), +0x3A7 (u8, W=1),
 *            +0x3AA (u8 body heading, R/W), +0x3AE (s16 stamina, R/W),
 *            +0x3B0 (u8 substate timer, R/W), +0x3B4 (u8 max speed, R),
 *            +0x3C8/+0x3CA (s16 target x/z, R/W), +0x3CC (u16 charge/press counter, R/W),
 *            +0x3D0 (u8 intercept/commit flag, R/W), +0x3D1 (u8, W=1).
 *   - ball:  +0x02/+0x06 (s16 x/z, R), +0x3A0 (u32 speed, R), +0x3A8 (u8 heading, R).
 *   - opp:   +0x02/+0x06 (s16 x/z, R), +0x3A0 (u32 speed, R), +0x3B4 (u8 max speed, R).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - -0x1FE1 (-8161) = 32.64 m X; 0x12F5 (4853) = 19.41 m Z (penalty-area exclusion).
 *   - 0x4000 (16384 = 128^2) -> 0.51 m squared bias favoring opponent proximity.
 *   - 0x2000 (8192) = ball speed threshold; 0x21 (33) = opponent speed threshold.
 *   - 0x200 (512) = 2.05 m X offset ahead of opponent/predicted ball.
 *   - 0x300 (768) = 3.07 m lead distance along ball heading in substate 3.
 *   - 0x40 (64) = 0.26 m close-range threshold; 0x200 (512) = 2.05 m approach threshold.
 *   - 0x81 (129) = 0.52 m re-chase distance in substate 2.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];
extern unsigned char D_800C6D64[];
extern unsigned char D_800C6D66[];

extern void func_8005BD1C(unsigned char *actor);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_80042B44(unsigned char *actor);
extern int func_80042A60(unsigned char *actor);
extern int func_8001E694(int angle, int scale);
extern int func_8001E5A4(int angle, int scale);
extern void func_800484B8(unsigned char *actor, int x, int z, int max_spd);
extern int func_8004419C(unsigned char *actor, int ball_angle);
extern unsigned int func_800B1128(unsigned int val);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_80049F90(unsigned char *actor);
extern int func_8005B7CC(int target, int cur, int step);
extern int func_80043018(unsigned char *actor, int ball_angle, int dist);
extern void func_80048FC4(unsigned char *actor, int tgt_angle, int ball_angle, int dist);
extern void func_80048AD8(unsigned char *actor, int tgt_angle, int ball_angle, int dist);
extern void func_800479B8(unsigned char *actor, int a1);
extern void func_80048D94(unsigned char *actor, int move_angle, int ball_angle, int a3);
extern void func_80042BE0(unsigned char *actor, unsigned char *target_actor, int ball_angle);
extern void func_8005B908(unsigned char *actor);

void func_800420E4(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *ball;
    /* UNCLEAR: target_actor ($s7) is not written on the owner==0 && func_8004419C!=0 path */
    unsigned char *target_actor = (unsigned char *)0;
    unsigned int actor_id;
    unsigned int phase;
    unsigned int owner;
    int ball_angle;
    int tgt_angle;
    int tgt_x, tgt_z;
    int intercept_flag;
    unsigned int dist;
    int dx, dz;

    B(actor, 0x3A7) = 1;
    B(actor, 0x3D1) = 1;
    func_8005BD1C(actor);

    actor_id = B(actor, 0x38D);
    if (actor_id == B(scratch, 0x2A9)) {
        B(actor, 0x396) = 0x1B;
        B(actor, 0x397) = 1;
        func_8005B908(actor);
        return;
    }

    phase = B(scratch, 0x29A);
    if (phase != 1 || B(scratch, 0x2FC) != 0 ||
        actor_id != B(scratch, 0x31E + (B(actor, 0x399) << 1))) {
        B(actor, 0x396) = 1;
        B(actor, 0x397) = 0;
        func_8005B908(actor);
        return;
    }

    H(actor, 0x3AE) = (unsigned short)(H(actor, 0x3AE) - 1);
    H(actor, 0x3CC) = (unsigned short)(H(actor, 0x3CC) + 1);
    ball_angle = func_8005B36C(S(actor, 2), S(actor, 6), S(scratch, 0x36), S(scratch, 0x66));

    owner = B(scratch, 0x2A9);
    if (owner != 0) {
        int reuse_prev = 0;
        intercept_flag = 0;
        target_actor = D_800FF748 + (owner % 24) * 1000;

        if (B(actor, 0x397) == phase) {
            if (S(scratch, 0x31C) < (func_80042B44(actor) & 0xFF)) {
                tgt_x = H(actor, 0x3C8);
                tgt_z = H(actor, 0x3CA);
                tgt_angle = B(actor, 0x3A4);
                reuse_prev = 1;
            }
        }
        if (!reuse_prev) {
            int lead = func_80042A60(actor);
            ball = (unsigned char *)W(scratch, 0x2F4);
            tgt_x = (H(scratch, 0x36) + func_8001E694(B(ball, 0x3A8), lead)) & 0xFFFF;
            ball = (unsigned char *)W(scratch, 0x2F4);
            tgt_z = (H(scratch, 0x66) + func_8001E5A4(B(ball, 0x3A8), lead)) & 0xFFFF;
            H(actor, 0x3C8) = (unsigned short)tgt_x;
            H(actor, 0x3CA) = (unsigned short)tgt_z;
            tgt_angle = func_8005B36C(S(actor, 2), S(actor, 6), (signed short)tgt_x, (signed short)tgt_z);
        }
    } else {
        func_800484B8(actor, S(actor, 2), S(actor, 6), B(actor, 0x3B4));
        tgt_x = H(scratch, 0xF8);
        tgt_z = H(scratch, 0xFC);

        if (func_8004419C(actor, ball_angle & 0xFF) != 0) {
            intercept_flag = 1;
            B(actor, 0x3D0) = 1;
            H(actor, 0x3C8) = H(actor, 2);
            H(actor, 0x3CA) = H(actor, 6);
        } else {
            int opp_idx = B(scratch, 0x31E + ((B(actor, 0x399) ^ 1) << 1));
            unsigned int actor_pred_sq;
            int pred_x, opp_x, opp_z;
            int in_box_x, abs_z;

            target_actor = D_800FF748 + (opp_idx % 24) * 1000;
            func_800484B8(target_actor, S(target_actor, 2), S(target_actor, 6), B(target_actor, 0x3B4));
            intercept_flag = 0;

            dx = S(actor, 2) - (signed short)tgt_x;
            dz = S(actor, 6) - (signed short)tgt_z;
            actor_pred_sq = (unsigned int)(dx * dx + dz * dz);

            pred_x = S(scratch, 0xF8);
            in_box_x = (B(actor, 0x399) != 0) ? (-pred_x < -0x1FE1) : (pred_x < -0x1FE1);
            abs_z = S(scratch, 0xFC);
            if (abs_z < 0) abs_z = -abs_z;

            if (in_box_x && abs_z < 0x12F5) {
                B(actor, 0x3D0) = 0;
            } else {
                int ox, oz;
                unsigned int opp_pred_sq;
                opp_x = S(target_actor, 2);
                opp_z = S(target_actor, 6);
                ox = opp_x - pred_x;
                oz = opp_z - S(scratch, 0xFC);
                opp_pred_sq = (unsigned int)(ox * ox + oz * oz);

                if (opp_pred_sq + 0x4000U < actor_pred_sq) {
                    int abx, abz, obx, obz;
                    ball = (unsigned char *)W(scratch, 0x2F4);
                    abx = S(actor, 2) - S(ball, 2);
                    abz = S(actor, 6) - S(ball, 6);
                    obx = opp_x - S(ball, 2);
                    obz = opp_z - S(ball, 6);
                    if ((obx * obx + obz * obz + 0x4000) < (abx * abx + abz * abz) &&
                        W(ball, 0x3A0) >= 0x2000U) {
                        if (W(target_actor, 0x3A0) < 0x21U) {
                            tgt_x = (B(actor, 0x399) != 0) ? (opp_x + 0x200) : (opp_x - 0x200);
                            tgt_z = H(target_actor, 6);
                        } else {
                            tgt_x = (B(actor, 0x399) != 0) ? (pred_x + 0x200) : (pred_x - 0x200);
                            tgt_z = H(scratch, 0xFC);
                        }
                        B(actor, 0x3D0) = 1;
                    } else {
                        B(actor, 0x3D0) = 0;
                    }
                } else {
                    B(actor, 0x3D0) = 0;
                }
            }
            H(actor, 0x3C8) = (unsigned short)tgt_x;
            H(actor, 0x3CA) = (unsigned short)tgt_z;
        }
        tgt_angle = func_8005B36C(S(actor, 2), S(actor, 6), (signed short)tgt_x, (signed short)tgt_z);
    }

    dx = S(actor, 2) - (signed short)tgt_x;
    dz = S(actor, 6) - (signed short)tgt_z;
    dist = func_800B1128((unsigned int)(dx * dx + dz * dz)) & 0xFFE0;

    switch (B(actor, 0x397)) {
    case 0:
        H(actor, 0x3CC) = 0;
        B(actor, 0x3B0) = 0;
        B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
        /* fall through */

    case 1:
        if ((intercept_flag & 0xFF) != 0) {
            if (S(scratch, 0x31C) >= 2) {
                int p_ang = func_8005B36C(S(actor, 2), S(actor, 6), S(scratch, 0xF8), S(scratch, 0xFC));
                unsigned int diff = (unsigned int)((p_ang - B(actor, 0x3AA)) & 0xFF);
                int need_reset = 0;
                if (diff < 0x81U) {
                    p_ang = func_8005B36C(S(actor, 2), S(actor, 6), S(scratch, 0xF8), S(scratch, 0xFC));
                    if (((p_ang - B(actor, 0x3AA)) & 0xFF) >= 0x40) {
                        need_reset = 1;
                    }
                } else {
                    p_ang = func_8005B36C(S(actor, 2), S(actor, 6), S(scratch, 0xF8), S(scratch, 0xFC));
                    if (((B(actor, 0x3AA) - p_ang) & 0xFF) >= 0x40) {
                        need_reset = 1;
                    }
                }
                if (need_reset && H(actor, 0x38E) != 1) {
                    func_8005BA30(actor, 1, 0);
                }
            }
            func_80049F90(actor);
        } else {
            unsigned int thresh;
            if (B(scratch, 0x2A9) == 0 && B(actor, 0x3D0) == 0) {
                thresh = 0x40U;
            } else {
                thresh = H(D_800C6D64, B(scratch, 0x328) * 4);
            }

            if (dist < thresh) {
                func_8005BA30(actor, 0xE, 0xF);
                B(actor, 0x3AA) = (unsigned char)func_8005B7CC(ball_angle & 0xFF, B(actor, 0x3AA), 0x10);
                if (W(actor, 0x3A0) < 0x13U) {
                    W(actor, 0x3A0) = 0;
                } else {
                    W(actor, 0x3A0) -= 0x12;
                }
                B(actor, 0x3B0) = 4;
                B(actor, 0x397) = 2;
            } else if (func_80043018(actor, ball_angle & 0xFF, dist) != 0) {
                B(actor, 0x397) = 3;
            } else {
                int b_dir;
                unsigned int diff;
                int use_8AD8;
                ball = (unsigned char *)W(scratch, 0x2F4);
                b_dir = B(ball, 0x3A8);
                diff = (unsigned int)((b_dir - ball_angle) & 0xFF);
                if (diff < 0x81U) {
                    use_8AD8 = ((int)diff < 0x70) || (dist >= 0x200U);
                } else {
                    use_8AD8 = (((ball_angle - b_dir) & 0xFF) < 0x70) || (dist >= 0x200U);
                }
                if (use_8AD8) {
                    func_80048AD8(actor, tgt_angle & 0xFF, ball_angle & 0xFF, dist & 0xFFFF);
                } else {
                    func_80048FC4(actor, tgt_angle & 0xFF, ball_angle & 0xFF, dist & 0xFFFF);
                }
            }
        }
        break;

    case 2:
        H(actor, 0x3CC) = (unsigned short)(H(actor, 0x3CC) + 3);
        func_8005BA30(actor, 0xE, 0xF);
        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(ball_angle & 0xFF, B(actor, 0x3AA), 0x10);
        B(actor, 0x3A4) = (unsigned char)tgt_angle;
        if (W(actor, 0x3A0) < 0x13U) {
            W(actor, 0x3A0) = 0;
        } else {
            W(actor, 0x3A0) -= 0x12;
        }
        if ((intercept_flag & 0xFF) == 0) {
            if (B(actor, 0x3B0) != 0) {
                B(actor, 0x3B0) = (unsigned char)(B(actor, 0x3B0) - 1);
            } else if (B(scratch, 0x2A9) == 0) {
                if (dist >= 0x81U) {
                    B(actor, 0x397) = 1;
                }
            } else {
                if (H(D_800C6D66, B(scratch, 0x328) * 4) < dist) {
                    B(actor, 0x397) = 1;
                }
            }
        }
        break;

    case 3:
        H(actor, 0x3AE) = (unsigned short)(H(actor, 0x3AE) - 2);
        if (B(scratch, 0x2A9) != 0) {
            ball = (unsigned char *)W(scratch, 0x2F4);
            H(scratch, 0xF8) = (unsigned short)(H(scratch, 0x36) + func_8001E694(B(ball, 0x3A8), 0x300));
            ball = (unsigned char *)W(scratch, 0x2F4);
            H(scratch, 0xFC) = (unsigned short)(H(scratch, 0x66) + func_8001E5A4(B(ball, 0x3A8), 0x300));
        } else {
            H(scratch, 0xF8) = (unsigned short)tgt_x;
            H(scratch, 0xFC) = (unsigned short)tgt_z;
        }
        func_800479B8(actor, 1);
        dx = S(actor, 2) - S(scratch, 0xF8);
        dz = S(actor, 6) - S(scratch, 0xFC);
        if (func_80043018(actor, ball_angle & 0xFF, dx * dx + dz * dz) == 0) {
            B(actor, 0x3B0) = 0;
            B(actor, 0x397) = 4;
        }
        break;

    case 4:
        func_80048D94(actor, B(actor, 0x3A4), ball_angle & 0xFF, 0xFF);
        B(actor, 0x3B0) = (unsigned char)(B(actor, 0x3B0) + 1);
        if (B(actor, 0x3B0) >= 4) {
            B(actor, 0x397) = 1;
        }
        break;
    }

    func_80042BE0(actor, target_actor, ball_angle & 0xFF);
    func_8005B908(actor);
}
