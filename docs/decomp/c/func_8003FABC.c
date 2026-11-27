/*
 * func_8003FABC
 *
 * Purpose:
 *   Evaluates and triggers a player's loose-ball / pass-interception or
 *   dribble-challenge action toward the predicted ball position (SCRATCH+0x42/0x72
 *   or SCRATCH+0x4A/0x7A), selecting one of three animations/actions:
 *     - anim 0x4D (77), substate 3: stretch/intercept touch (returns 1)
 *     - anim 0x4C (76), substate 1: side-step block/deflect (returns 1)
 *     - anim 0x1E (30), substate 1: normal run-to-ball approach (returns 1)
 *   Returns 0 if a closer/assigned actor already beats this actor to the target.
 *
 * Arguments:
 *   actor (unsigned char *) - pointer to 1000-byte player actor struct
 *
 * Return value:
 *   int - 1 if an action was initiated, 0 if aborted (another actor is closer)
 *
 * Reads / Writes:
 *   Actor fields:
 *     +0x002 (s16) x (R)
 *     +0x006 (s16) z (R)
 *     +0x008 (s32) velocity x (W)
 *     +0x010 (s32) velocity z (W)
 *     +0x38D (u8)  actor id (R)
 *     +0x392 (u8)  team slot 0..10 (R)
 *     +0x397 (u8)  substate (W: 1 or 3)
 *     +0x398 (u8)  side 0/1 (R)
 *     +0x399 (u8)  attack direction flag (R)
 *     +0x3A0 (u32) speed (W: 0 in anim 0x4C path)
 *     +0x3A4 (u8)  move heading (R/W)
 *     +0x3AA (u8)  body heading (R/W)
 *     +0x3AB (u8)  target/facing heading (W)
 *     +0x3AC (u8)  mirror/foot flag (R/W)
 *     +0x3B0 (u8)  motion/step mode flag (W: 2 in anim 0x4C path)
 *     +0x3B4 (u8)  max speed (R)
 *     +0x3CA (s16) computed move speed (W)
 *   Scratchpad (0x1F800000):
 *     +0x042 / +0x072 (s16) predicted ball x / z at first lookahead (R)
 *     +0x04A / +0x07A (s16) predicted ball x / z at second lookahead (R)
 *     +0x0F0 / +0x0F4 (s16) scratch reference x / z (R)
 *     +0x2A9 (u8)  ball owner (0 = free) (R)
 *     +0x2CF (u8[2]) per-side possession flags (R)
 *     +0x2F4 (u8*) ptr to ball actor (R)
 *     +0x31C (s16) ball timer, frames since last touch (R)
 *   Globals:
 *     D_800FF748 (u8[])  actor array (1000-byte stride)
 *     D_8010C330 (u32[]) player stat word table (side*264 + slot*12)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x24000 = 147456 = 384^2 (~1.536 m radius) - opponent proximity check
 *   0x78FFF = 495615 (~704 raw = 2.816 m)^2 - max dist^2 for anim 0x4C block
 *   0x3320  = 13088 raw (52.352 m) - goal line X
 *   0x1FE2  = 8162 raw (32.648 m) - attacking X limit for anim 0x4C block
 *   0x51 = 81 (~114 deg), 0x50 = 80 (~112.5 deg), 0x18 = 24 (~33.75 deg)
 *   0x28 = 40 (56.25 deg/frame turn step), 0x30 = 48 (67.5 deg side offset)
 *   9 frames (0.3 s) ball timer threshold, /7 and /11 frame step divisors
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010C330[];

extern unsigned int func_8005CE30(unsigned int);
extern unsigned int func_8005B36C(int, int, int, int);
extern unsigned int func_80052A68(unsigned char *, int, unsigned int);
extern void func_8005BA30(unsigned char *, int, int);
extern unsigned int func_8005B7CC(unsigned int, unsigned int, int);
extern void func_8005C2AC(unsigned char *, int, int, unsigned int, unsigned int);
extern unsigned int func_800B1128(unsigned int);
extern int func_8001E694(unsigned int, unsigned int);
extern int func_8001E5A4(unsigned int, unsigned int);

int func_8003FABC(unsigned char *actor)
{
    unsigned int other_idx;
    unsigned char *other_actor;
    int pred_x1;
    int pred_z1;
    unsigned int target_ang;
    unsigned int body_ang;
    unsigned int ang_var;
    unsigned int owner;
    unsigned int max_spd_adj;
    unsigned int ang_diff_move;
    int goal_x;
    unsigned int goal_ang_diff;
    int dx_self;
    int dz_self;
    int dx_other;
    int dz_other;
    unsigned int move_ang;
    unsigned int dist;
    unsigned int speed;
    int pred_x2;
    int pred_z2;
    int dx;
    int dz;
    int signed_x;
    unsigned char *ball;
    unsigned int sector;
    unsigned int facing_ang;
    unsigned int stat_word;

    other_idx = func_8005CE30(B(actor, 0x38D)) & 0xFF;
    other_actor = D_800FF748 + (other_idx % 24U) * 1000U;

    pred_x1 = H(SCRATCH, 0x42);
    pred_z1 = H(SCRATCH, 0x72);
    target_ang = func_8005B36C(S(actor, 2), S(actor, 6), (short)pred_x1, (short)pred_z1);

    body_ang = B(actor, 0x3AA);
    if (((target_ang - body_ang) & 0xFF) < 0x81U) {
        ang_var = target_ang - body_ang;
    } else {
        ang_var = body_ang - target_ang;
    }

    owner = B(SCRATCH, 0x2A9);
    if ((owner == 0 || B(actor, 0x38D) == owner)
        && (ang_var & 0xFF) < 0x51U
        && !(B(SCRATCH, 0x2CF + B(actor, 0x398)) != 0 && S(SCRATCH, 0x31C) < 9)
        && (func_80052A68(actor, 6, 0x24000) & 0xFF) == 0) {

        ang_diff_move = (B(actor, 0x3A4) - target_ang) & 0xFF;
        if (ang_diff_move < 0x81U) {
            max_spd_adj = (B(actor, 0x3B4) + 2U) - ((int)ang_diff_move >> 2);
        } else {
            max_spd_adj = (B(actor, 0x3B4) + 2U) - (((int)((target_ang - B(actor, 0x3A4)) & 0xFF)) >> 2);
        }

        goal_x = (B(actor, 0x399) != 0) ? -0x3320 : 0x3320;
        /* UNCLEAR: GCC reused register $s3 (ang_var) for goal_ang here; if the check below fails,
         * the subsequent (< 0x50) branch and speed penalty read this overwritten value instead of ang_diff. */
        ang_var = func_8005B36C(S(actor, 2), S(actor, 6), goal_x, 0);

        goal_ang_diff = (target_ang - ang_var) & 0xFF;
        if (goal_ang_diff < 0x81U) {
            if ((int)goal_ang_diff < 0x18) {
                goto check_block_anim;
            }
        } else {
            if ((int)((ang_var - target_ang) & 0xFF) < 0x18) {
                goto check_block_anim;
            }
        }

        pred_x1 = (short)pred_x1;
        pred_z1 = (short)pred_z1;
        dx_self = S(actor, 2) - pred_x1;
        dz_self = S(actor, 6) - pred_z1;
        dx_other = S(other_actor, 2) - pred_x1;
        dz_other = S(other_actor, 6) - pred_z1;
        if (dx_other * dx_other + dz_other * dz_other < dx_self * dx_self + dz_self * dz_self) {
            return 0;
        }

        func_8005BA30(actor, 0x4D, 3);
        B(actor, 0x3AA) = func_8005B7CC(target_ang & 0xFF, B(actor, 0x3AA), 0x28);
        B(actor, 0x3AC) = ((ang_var - target_ang) & 0xFF) < 0x81U;
        func_8005C2AC(actor, 0x4D, 9, target_ang & 0xFF, B(actor, 0x3AC));

        move_ang = func_8005B36C(S(SCRATCH, 0xF0), S(SCRATCH, 0xF4), pred_x1, pred_z1);
        dx = S(SCRATCH, 0xF0) - pred_x1;
        dz = S(SCRATCH, 0xF4) - pred_z1;
        dist = func_800B1128((unsigned int)(dx * dx + dz * dz));
        speed = dist / 7U;
        if (max_spd_adj < speed) {
            speed = max_spd_adj;
        }

        SW(actor, 8) = func_8001E694(move_ang & 0xFF, speed);
        SW(actor, 16) = func_8001E5A4(move_ang & 0xFF, speed);
        H(actor, 0x3CA) = (unsigned short)speed;
        B(actor, 0x3AB) = (unsigned char)target_ang;
        B(actor, 0x397) = 3;
        return 1;
    }

check_block_anim:
    if ((ang_var & 0xFF) < 0x50U) {
        dx = S(actor, 2) - S(SCRATCH, 0x4A);
        dz = S(actor, 6) - S(SCRATCH, 0x7A);
        if (dx * dx + dz * dz <= 0x78FFF) {
            signed_x = (B(actor, 0x399) != 0) ? -S(actor, 2) : S(actor, 2);
            if (signed_x < 0x1FE2) {
                pred_x2 = H(SCRATCH, 0x4A);
                pred_z2 = H(SCRATCH, 0x7A);
                target_ang = func_8005B36C(S(actor, 2), S(actor, 6), (short)pred_x2, (short)pred_z2);

                ang_diff_move = (B(actor, 0x3A4) - ang_var) & 0xFF;
                if (ang_diff_move < 0x81U) {
                    max_spd_adj = (B(actor, 0x3B4) + 2U) - ((int)ang_diff_move >> 2);
                } else {
                    max_spd_adj = (B(actor, 0x3B4) + 2U) - (((int)((ang_var - B(actor, 0x3A4)) & 0xFF)) >> 2);
                }

                ball = (unsigned char *)W(SCRATCH, 0x2F4);
                sector = ((B(ball, 0x3A4) - target_ang + 8U) & 0xFF) >> 4;
                if ((int)sector >= 3 && ((int)sector < 6 || ((int)sector < 14 && (int)sector >= 11))) {
                    B(actor, 0x3AC) = (unsigned char)(sector >> 3);
                    func_8005BA30(actor, 0x4C, 2);

                    ang_var = func_8005B36C(S(actor, 2), S(actor, 6), S(SCRATCH, 0x4A), S(SCRATCH, 0x7A));
                    B(actor, 0x3AB) = (unsigned char)ang_var;
                    if (B(actor, 0x3AC) != 0) {
                        facing_ang = ((ang_var & 0xFF) - 0x30U) & 0xFF;
                    } else {
                        facing_ang = ((ang_var & 0xFF) + 0x30U) & 0xFF;
                    }
                    B(actor, 0x3AA) = func_8005B7CC(facing_ang, B(actor, 0x3AA), 0x28);
                    func_8005C2AC(actor, 0x4C, 0xC, ang_var & 0xFF, B(actor, 0x3AC));

                    pred_x2 = (short)pred_x2;
                    pred_z2 = (short)pred_z2;
                    move_ang = func_8005B36C(S(SCRATCH, 0xF0), S(SCRATCH, 0xF4), pred_x2, pred_z2);
                    dx = S(SCRATCH, 0xF0) - pred_x2;
                    dz = S(SCRATCH, 0xF4) - pred_z2;
                    dist = func_800B1128((unsigned int)(dx * dx + dz * dz));
                    speed = dist / 11U;
                    if (max_spd_adj < speed) {
                        speed = max_spd_adj;
                    }

                    SW(actor, 8) = func_8001E694(move_ang & 0xFF, speed);
                    SW(actor, 16) = func_8001E5A4(move_ang & 0xFF, speed);
                    B(actor, 0x3B0) = 2;
                    W(actor, 0x3A0) = 0;
                    B(actor, 0x397) = 1;
                    return 1;
                }
            }
        }
    }

    if (B(SCRATCH, 0x2A9) == 0) {
        pred_x1 = S(SCRATCH, 0x42);
        pred_z1 = S(SCRATCH, 0x72);
        dx_self = S(actor, 2) - pred_x1;
        dz_self = S(actor, 6) - pred_z1;
        dx_other = S(other_actor, 2) - pred_x1;
        dz_other = S(other_actor, 6) - pred_z1;
        if (dx_other * dx_other + dz_other * dz_other < dx_self * dx_self + dz_self * dz_self) {
            return 0;
        }
    }

    func_8005BA30(actor, 0x1E, 0);
    stat_word = W(D_8010C330, B(actor, 0x392) * 12 + B(actor, 0x398) * 264);
    B(actor, 0x3AC) = (unsigned char)((stat_word >> 4) & 1U);

    ang_diff_move = (B(actor, 0x3A4) - target_ang) & 0xFF;
    if (ang_diff_move < 0x81U) {
        max_spd_adj = (B(actor, 0x3B4) + 2U) - ((int)ang_diff_move >> 2);
    } else {
        max_spd_adj = (B(actor, 0x3B4) + 2U) - (((int)((target_ang - B(actor, 0x3A4)) & 0xFF)) >> 2);
    }

    func_8005C2AC(actor, 0x1E, 6, target_ang & 0xFF, B(actor, 0x3AC));

    move_ang = func_8005B36C(S(SCRATCH, 0xF0), S(SCRATCH, 0xF4), S(SCRATCH, 0x42), S(SCRATCH, 0x72));
    B(actor, 0x3AA) = func_8005B7CC(target_ang & 0xFF, B(actor, 0x3AA), 0x28);

    dx = S(SCRATCH, 0xF0) - S(SCRATCH, 0x42);
    dz = S(SCRATCH, 0xF4) - S(SCRATCH, 0x72);
    dist = func_800B1128((unsigned int)(dx * dx + dz * dz));
    speed = dist / 7U;
    if (max_spd_adj < speed) {
        speed = max_spd_adj;
    }

    SW(actor, 8) = func_8001E694(move_ang & 0xFF, speed);
    SW(actor, 16) = func_8001E5A4(move_ang & 0xFF, speed);
    H(actor, 0x3CA) = (unsigned short)speed;
    B(actor, 0x3AB) = (unsigned char)target_ang;
    B(actor, 0x3A4) = (unsigned char)move_ang;
    B(actor, 0x397) = 1;
    return 1;
}
