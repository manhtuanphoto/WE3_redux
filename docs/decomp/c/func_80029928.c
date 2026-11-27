/*
 * func_80029928 - Player dribble/turn-to-desired-heading decision handler.
 *
 * Purpose:
 *   Selects a dribbling, cut-back, or back-heel/turn animation and substate (3, 5, 7, 8,
 *   or 9) based on the 16-sector angular difference between the desired heading
 *   (func_8002A8F4) and the player's current/ball-oriented heading, taking into account
 *   proximity to the nearest opponent and the opponent's goal.
 *
 * Arguments:
 *   actor (unsigned char*): dribbling player actor struct (1000 bytes).
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F800040 / 0x1F800070 (s16, R): predicted ball X/Z.
 *   - 0x1F8002A8 (u8, R/W): last-touch actor ID.
 *   - 0x1F8002F4 (u8*, R): pointer to ball actor.
 *
 * Actor / global fields reads/writes:
 *   - actor: +0x02/+0x06 (s16 x/z, R), +0x38D (u8 id, R), +0x392 (u8 team slot, R),
 *            +0x397 (u8 substate, W), +0x398 (u8 side, R), +0x399 (u8 attack dir, R),
 *            +0x3A0 (u32 speed, R/W), +0x3AA (u8 body heading, R/W),
 *            +0x3AB (u8 target heading, W), +0x3AC (u8 mirror/foot, W),
 *            +0x3B4 (u8 max speed, R), +0x3C2 (s16, W=2), +0x3C8 (s16 dribble touch timer, W),
 *            +0x3D0 (u8, W=4/6), +0x3D1 (u8, W=4/2/0).
 *   - nearest_opp (D_800FF748 + (id % 24) * 1000): +0x02/+0x06 (s16 x/z, R).
 *   - ball: +0x396 (u8 state, W=3).
 *   - D_8010C32E[] / D_8010C330[] (u16/u32, R): player stat tables indexed by slot*12 + side*264.
 *   - D_800C6C4B (u8, R): turn-heading offset constant.
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x3320 (13088) = 52.35 m opponent goal-line X.
 *   - 0x78FFFF (7929855, ~2816^2) = 11.26 m squared close-pressure/goal distance threshold.
 *   - 0x23FFFFF (37748735, ~6144^2) = 24.58 m squared open-space distance threshold.
 *   - 0x30 (48) = minimum dribble speed when last-touch; 0x31 (49) = speed threshold for 0x3D0.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];
extern unsigned char D_8010C32E[];
extern unsigned char D_8010C330[];
extern unsigned char D_800C6C4B;

extern int func_8002A8F4(unsigned char *actor);
extern int func_8005CE30(int actor_id);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8005B7CC(int target, int cur, int step);
extern int func_8002BAEC(unsigned char *actor, unsigned char *opp, int heading);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_80049DF0(unsigned char *actor, int heading, int a2, int a3);
extern int func_8001E6B8(int n);
extern int func_8005385C(unsigned char *actor, int a1);
extern void func_8004B0F0(unsigned char *actor, int heading, int power, int lift);
extern void func_8004B414(void);

void func_80029928(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *nearest_opp;
    unsigned char *ball;
    int desired_heading;
    int opp_id;
    int dx, dz, goal_dx;
    unsigned int min_dist_sq, goal_dist_sq;
    int ball_ang, ref_heading, cur_body, diff;
    unsigned int sector;
    int out_heading;
    int row;
    int foot;

    B(actor, 0x3D1) = 4;
    desired_heading = func_8002A8F4(actor);

    opp_id = func_8005CE30(B(actor, 0x38D)) & 0xFF;
    nearest_opp = D_800FF748 + ((unsigned int)opp_id % 24U) * 1000;

    dx = S(actor, 2) - S(nearest_opp, 2);
    dz = S(actor, 6) - S(nearest_opp, 6);
    min_dist_sq = (unsigned int)(dx * dx + dz * dz);

    goal_dx = (B(actor, 0x399) != 0) ? (S(actor, 2) + 0x3320) : (S(actor, 2) - 0x3320);
    goal_dist_sq = (unsigned int)(goal_dx * goal_dx + S(actor, 6) * S(actor, 6));
    if (goal_dist_sq < min_dist_sq) {
        min_dist_sq = goal_dist_sq;
    }

    ball_ang = func_8005B36C(S(actor, 2), S(actor, 6), S(scratch, 0x40), S(scratch, 0x70));
    ref_heading = ball_ang;
    cur_body = B(actor, 0x3AA);
    diff = (ref_heading - cur_body) & 0xFF;
    if ((unsigned int)diff < 0x81U) {
        if (diff >= 0x40) {
            ref_heading = cur_body;
        }
    } else {
        if (((cur_body - ref_heading) & 0xFF) >= 0x40) {
            ref_heading = cur_body;
        }
    }

    sector = (unsigned int)(((desired_heading - ref_heading) + 8) & 0xFF) >> 4;
    if (sector >= 0x10U) {
        goto finish_heading;
    }

    switch (sector) {
    case 0:
    case 1:
    case 2:
    case 14:
    case 15:
        row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        B(actor, 0x3AC) = (unsigned char)((W(D_8010C330, row) >> 4) & 1);
        out_heading = func_8005B7CC(desired_heading & 0xFF, ref_heading & 0xFF, 0xA);
        if (min_dist_sq <= 0x78FFFFU) {
            if (func_8002BAEC(actor, nearest_opp, desired_heading) != 0) {
                S(actor, 0x3C2) = 2;
            }
            func_8005BA30(actor, 0x10, 5);
            func_80049DF0(actor, out_heading & 0xFF, 9, 4);
            B(actor, 0x397) = 5;
        } else {
            if (B(actor, 0x38D) == B(scratch, 0x2A8) && W(actor, 0x3A0) < 0x30U) {
                W(actor, 0x3A0) = 0x30;
            }
            if (min_dist_sq > 0x23FFFFFU || (out_heading & 0xFF) == 2 || (out_heading & 0xFF) == 0xE) {
                func_8005BA30(actor, 0xF, 0x15);
                if (W(actor, 0x3A0) < B(actor, 0x3B4)) {
                    S(actor, 0x3C8) = (short)(func_8001E6B8(8) + 0xF0);
                } else {
                    S(actor, 0x3C8) = (short)(func_8001E6B8(8) + 0xF8);
                }
                func_80049DF0(actor, out_heading & 0xFF, 0x1A, 5);
                B(actor, 0x397) = 3;
            } else {
                func_8005BA30(actor, 0xF, 0x10);
                if (W(actor, 0x3A0) < B(actor, 0x3B4)) {
                    S(actor, 0x3C8) = (short)(func_8001E6B8(0x10) + 0xF8);
                } else {
                    S(actor, 0x3C8) = (short)(func_8001E6B8(0x10) + 0x100);
                }
                func_80049DF0(actor, out_heading & 0xFF, 0x1A, 0xA);
                B(actor, 0x397) = 3;
            }
        }
        break;

    case 3:
    case 4:
    case 5:
    case 11:
    case 12:
    case 13:
        if (min_dist_sq <= 0x78FFFFU) {
            row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
            if ((9 - (int)(H(D_8010C32E, row) & 0xF)) < 3) {
                if (func_8005385C(actor, 1) == 0) {
                    return;
                }
                B(actor, 0x3AC) = (unsigned char)(((sector & 0xFFU) >> 3) ^ 1);
                func_8005BA30(actor, 0x12, 0);
                out_heading = func_8005B7CC((desired_heading + 0x28) & 0xFF, ref_heading & 0xFF, 0xC);
                func_80049DF0(actor, out_heading & 0xFF, 4, 4);
                if (func_8002BAEC(actor, nearest_opp, desired_heading & 0xFF) != 0) {
                    S(actor, 0x3C2) = 2;
                }
                B(actor, 0x397) = 7;
                break;
            }
        }
        B(actor, 0x3AC) = (unsigned char)((sector & 0xFFU) >> 3);
        func_8005BA30(actor, 0xF, 0x15);
        out_heading = func_8005B7CC(desired_heading & 0xFF, ref_heading & 0xFF, 0x10);
        B(actor, 0x3AB) = (unsigned char)out_heading;
        out_heading = func_8005B7CC(out_heading & 0xFF, B(actor, 0x3AA), 0x10);
        B(actor, 0x3AA) = (unsigned char)out_heading;
        func_80049DF0(actor, out_heading & 0xFF, 0x1A, 5);
        S(actor, 0x3C8) = 0xD0;
        B(actor, 0x397) = 3;
        return;

    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        if (func_8005385C(actor, 1) == 0) {
            B(actor, 0x3D1) = 2;
            return;
        }
        ball = (unsigned char *)W(scratch, 0x2F4);
        B(scratch, 0x2A8) = B(actor, 0x38D);
        B(ball, 0x396) = 3;
        if (W(actor, 0x3A0) == 0) {
            foot = ((ref_heading - desired_heading) & 0xFF) < 0x81U;
            B(actor, 0x3AC) = (unsigned char)foot;
            func_8005BA30(actor, 0x54, 0);
            if (B(actor, 0x3AC) != 0) {
                out_heading = (ref_heading & 0xFF) - 0x54;
            } else {
                out_heading = (ref_heading & 0xFF) + 0x54;
            }
            func_8004B0F0(actor, out_heading & 0xFF, 0x1A, 0);
            func_8004B414();
            B(actor, 0x3D1) = 0;
            if (B(actor, 0x3AC) != 0) {
                out_heading = (ref_heading & 0xFF) - D_800C6C4B;
            } else {
                out_heading = (ref_heading & 0xFF) + D_800C6C4B;
            }
            func_80049DF0(actor, out_heading & 0xFF, 7, 7);
            out_heading = ref_heading;
            B(actor, 0x397) = 9;
        } else {
            func_8005BA30(actor, 0x16, 0);
            B(actor, 0x3D0) = (W(actor, 0x3A0) < 0x31U) ? 4 : 6;
            B(actor, 0x397) = 8;
            out_heading = sector;
        }
        break;
    }

finish_heading:
    B(actor, 0x3AA) = (unsigned char)func_8005B7CC(out_heading & 0xFF, B(actor, 0x3AA), 0xA);
    B(actor, 0x3AB) = (unsigned char)out_heading;
}
