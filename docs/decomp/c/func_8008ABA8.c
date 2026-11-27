/*
 * func_8008ABA8 - Goalkeeper diving/interception trigger check and initializer (state 0x91).
 *
 * Purpose:
 *   Evaluates whether a goalkeeper can intercept or dive for the ball at predicted
 *   trajectory index (8 - frame_idx) in open play: verifies ball height, penalty-box
 *   bounds, teammate/opponent proximity, facing cone (< 0x31 = 69 deg), and reachability
 *   against ball owner or free-ball speed; if valid, computes dive velocity, starts
 *   anim 0x67 at frame_idx, and enters state 0x91.
 *
 * Arguments:
 *   actor (unsigned char*): goalkeeper actor struct (1000 bytes).
 *   frame_idx (int): starting animation frame / lookahead selector (target slot 8 - frame_idx,
 *                    time horizon 9 - frame_idx frames).
 *
 * Return value:
 *   int: 1 if dive/interception state 0x91 was started, 0 otherwise.
 *
 * Scratchpad reads/writes:
 *   - 0x1F800036 + (8-frame_idx)*2 (s16, R): predicted ball X; 0x46 (s16, R): slot 8 X.
 *   - 0x1F80004C (s16, R): base ball X; 0x1F80004E + (8-frame_idx)*2 (s16, R): predicted ball Y.
 *   - 0x1F80005E (s16, R): predicted ball Y at slot 8.
 *   - 0x1F800066 + (8-frame_idx)*2 (s16, R): predicted ball Z; 0x76 (s16, R): slot 8 Z.
 *   - 0x1F8000E0 (u16, R): ball/trajectory block flag (must be 0).
 *   - 0x1F8000F0 / 0x1F8000F4 (s16, R): scratchpad reference/origin X/Z.
 *   - 0x1F8000F8 / 0x1F8000FC (s16, R): predicted target X/Z set by func_8004840C.
 *   - 0x1F80029A (u8, R): match phase (must be 1 = open play).
 *   - 0x1F8002A9 (u8, R): ball owner actor ID (0 = free).
 *   - 0x1F8002AA (u8, R): previous ball owner actor ID.
 *   - 0x1F8002B8 / 0x1F8002BA (s16, R): target X/Z (0x7FFF = unset).
 *   - 0x1F8002F4 (u8*, R): pointer to ball actor.
 *   - 0x1F80031C (s16, R): ball timer (frames since last touch).
 *   - 0x1F80031E[2] (u8, R): nearest teammate ID per attack direction.
 *
 * Actor / global fields reads/writes:
 *   - actor: +0x02/+0x06 (s16 x/z, R), +0x04 (s16 y, W=0), +0x08/+0x10 (s32 vx/vz, W),
 *            +0x38D (u8 id, R), +0x392 (u8 team slot, R), +0x396 (u8 state, W=0x91),
 *            +0x397 (u8 substate, W=0), +0x398 (u8 side, R), +0x399 (u8 attack dir, R),
 *            +0x3A0 (u32 speed, R), +0x3AA (u8 body heading, R), +0x3AB (u8 target heading, W),
 *            +0x3AC (u8 mirror/foot, W), +0x3B4 (u8 max speed, R), +0x3B9 (u8 reaction stat, R),
 *            +0x3C2/+0x3C6 (s16 target x/z, W), +0x3CE (s16, W=0), +0x3D1 (u8, W=0).
 *   - owner (D_800FF748 + (owner_id % 24) * 1000): +0x02/+0x06 (s16 x/z, R), +0x396 (u8 state, R).
 *   - ball:  +0x02/+0x06 (s16 x/z, R), +0x08 (s32 vx, R).
 *   - D_8010C328[] (u32, R): player stat word (low nibble indexes D_800CA618[]).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - -0x60 (-96) = -0.384 m ball height limit; -0xC0 (-192) = -0.768 m dive-height limit.
 *   - 0x12F4 (4852) = 19.41 m |z| penalty-box half-width.
 *   - -0x2421 (-9249) = -37.00 m X boundary (own half/third); -0x3320 (-13088) = -52.35 m goal line.
 *   - 0x31 (49) = ~68.9 deg max angle difference from body heading.
 *   - 0x24000 (147456 = 384^2) = 1.54 m squared owner-to-ball distance.
 *   - 0xC400 (50176 = 224^2) = 0.90 m squared bonus reach margin; 0x80 (128) = 0.51 m reach pad.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];
extern unsigned char D_8010C328[];
extern unsigned int D_800CA618[];

extern int func_8004968C(unsigned char *actor, int x, int z);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8001E6B8(int n);
extern void func_8004840C(unsigned char *actor);
extern void func_8005C2AC(unsigned char *actor, int anim, int a2, int heading, int foot);
extern int func_800B1128(int val);
extern int func_8001E694(int angle, int scale);
extern int func_8001E5A4(int angle, int scale);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);

int func_8008ABA8(unsigned char *actor, int frame_idx)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *base = D_800FF748;
    unsigned char *ball;
    unsigned char *other;
    int slot;
    short pred_x, pred_z;
    int x_atk, x_cur, abs_z;
    int dx, dz;
    unsigned int dist_sq, other_dist_sq;
    int bx;
    int ang, cur_body, diff, angle_ok;
    int owner_id;
    int foot;
    int dist;
    unsigned int step_spd, max_spd;
    int ref_ang;

    if (B(scratch, 0x29A) != 1) {
        return 0;
    }

    if (B(scratch, 0x2A9) == 0 && B(actor, 0x38D) != B(scratch, 0x2AA)) {
        if (S(scratch, 0x31C) < (19 - (int)(B(actor, 0x3B9) / 10U))) {
            return 0;
        }
    }

    slot = (8 - frame_idx) * 2;
    if (S(scratch, 0x4E + slot) < -0x60) {
        return 0;
    }

    pred_x = S(scratch, 0x36 + slot);
    pred_z = S(scratch, 0x66 + slot);
    if (H(scratch, 0xE0) != 0) {
        return 0;
    }

    abs_z = pred_z;
    if (abs_z < 0) {
        abs_z = -abs_z;
    }
    if (abs_z >= 0x12F4) {
        return 0;
    }

    x_atk = (B(actor, 0x399) != 0) ? -pred_x : pred_x;
    if (x_atk >= -0x2421) {
        return 0;
    }

    ball = (unsigned char *)W(scratch, 0x2F4);
    x_cur = S(scratch, 0x4C) + (SW(ball, 8) >> 5);
    if (B(actor, 0x399) != 0) {
        x_cur = -x_cur;
    }
    if (x_cur < -0x3320) {
        return 0;
    }

    dx = S(actor, 2) - pred_x;
    dz = S(actor, 6) - pred_z;
    dist_sq = (unsigned int)(dx * dx + dz * dz);

    other = base + ((unsigned int)B(scratch, 0x31E + B(actor, 0x399) * 2) % 24U) * 1000;
    dx = S(other, 2) - pred_x;
    dz = S(other, 6) - pred_z;
    other_dist_sq = (unsigned int)(dx * dx + dz * dz);
    if (other_dist_sq < dist_sq) {
        return 0;
    }

    bx = S(scratch, 0x2B8);
    if (bx != 0x7FFF) {
        if ((unsigned int)func_8004968C(actor, bx, S(scratch, 0x2BA)) < dist_sq) {
            return 0;
        }
    }

    ang = func_8005B36C(S(actor, 2), S(actor, 6), pred_x, pred_z);
    cur_body = B(actor, 0x3AA);
    diff = (ang - cur_body) & 0xFF;
    if ((unsigned int)diff < 0x81U) {
        angle_ok = (diff < 0x31);
    } else {
        angle_ok = (((cur_body - ang) & 0xFF) < 0x31);
    }
    if (!angle_ok) {
        return 0;
    }

    owner_id = B(scratch, 0x2A9);
    if (owner_id != 0 && B(actor, 0x38D) != owner_id) {
        other = base + ((unsigned int)owner_id % 24U) * 1000;
        if (B(other, 0x396) == 4 && func_8001E6B8(4) == 0) {
            ball = (unsigned char *)W(scratch, 0x2F4);
            dx = S(other, 2) - S(ball, 2);
            dz = S(other, 6) - S(ball, 6);
            if (dx * dx + dz * dz <= 0x24000) {
                int row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
                int thresh = (int)D_800CA618[W(D_8010C328, row) & 0xF];
                if ((unsigned int)(thresh * thresh) < dist_sq) {
                    return 0;
                }
                func_8004840C(actor);
                S(actor, 0x3C2) = S(scratch, 0xF8);
                S(actor, 0x3C6) = S(scratch, 0xFC);
                goto start_dive;
            }
        }

        max_spd = B(actor, 0x3B4);
        if ((unsigned int)(81 * max_spd * max_spd + 0xC400) < dist_sq) {
            return 0;
        }
        if (S(scratch, 0x5E) < -0xC0) {
            return 0;
        }
        S(actor, 0x3C2) = S(scratch, 0x46);
        S(actor, 0x3C6) = S(scratch, 0x76);
    } else {
        unsigned int cur_spd = W(actor, 0x3A0);
        int range;
        if (cur_spd >= 0x31U) {
            range = (int)cur_spd * (9 - frame_idx) + 0x80;
        } else {
            range = (9 - frame_idx) * 0x30 + 0x80;
        }
        if ((unsigned int)(range * range) < dist_sq) {
            return 0;
        }
        if (S(scratch, 0x4E + (8 - frame_idx) * 2) < -0xC0) {
            return 0;
        }
        S(actor, 0x3C2) = pred_x;
        S(actor, 0x3C6) = pred_z;
    }

start_dive:
    B(actor, 0x3AB) = (unsigned char)ang;
    foot = ((((B(actor, 0x399) << 7) - ang) & 0xFF) < 0x81U) ^ 1;
    B(actor, 0x3AC) = (unsigned char)foot;
    func_8005C2AC(actor, 0x67, 8, ang & 0xFF, foot);

    dx = S(scratch, 0xF0) - pred_x;
    dz = S(scratch, 0xF4) - pred_z;
    dist = func_800B1128(dx * dx + dz * dz);
    step_spd = (unsigned int)dist / (unsigned int)(9 - frame_idx);
    max_spd = B(actor, 0x3B4) + 8;
    if (step_spd > max_spd) {
        step_spd = max_spd;
    }

    ref_ang = func_8005B36C(S(scratch, 0xF0), S(scratch, 0xF4), pred_x, pred_z) & 0xFF;
    SW(actor, 8) = func_8001E694(ref_ang, (int)step_spd);
    SW(actor, 0x10) = func_8001E5A4(ref_ang, (int)step_spd);

    S(actor, 0x3CE) = 0;
    B(actor, 0x3D1) = 0;
    S(actor, 4) = 0;
    func_8005BA30(actor, 0x67, frame_idx & 0xFF);
    B(actor, 0x396) = 0x91;
    B(actor, 0x397) = 0;
    return 1;
}
