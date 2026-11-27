/*
 * func_8006DD64
 *
 * Purpose:
 *   Per-frame goalkeeper off-ball positioning and anticipation AI. Handles
 *   reaction delay after kicks/deflections, holding/returning to goal-line
 *   sweeper position when the ball is in the opponent's half, tracking the
 *   goal-mouth arc (or ball bounce target / nearest attacker) when defending,
 *   and selecting idle, walk, or sprint movement helpers.
 *
 * Arguments:
 *   actor (unsigned char *) - pointer to 1000-byte goalkeeper actor struct
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Actor fields:
 *     +0x002 (s16) x (R)
 *     +0x006 (s16) z (R)
 *     +0x38D (u8)  actor id (R)
 *     +0x38E (u16) current anim id (R)
 *     +0x392 (u8)  team slot (R)
 *     +0x396 (u8)  state (W)
 *     +0x397 (u8)  substate (W)
 *     +0x398 (u8)  side 0/1 (R)
 *     +0x399 (u8)  attack direction flag (R)
 *     +0x3A0 (u32) speed (R/W)
 *     +0x3A4 (u8)  move heading (W)
 *     +0x3A7 (u8)  written = 1 on entry (W)
 *     +0x3AA (u8)  body heading (R/W)
 *     +0x3B4 (u8)  max speed (R)
 *     +0x3B9 (u8)  response/stat*10 byte (R)
 *     +0x3BC (s16) shot/danger flag (R)
 *     +0x3D1 (u8)  written = 1 on entry (W)
 *   Scratchpad (0x1F800000):
 *     +0x036/+0x066 (s16) reference/ball x/z (R)
 *     +0x046/+0x076 (s16) current ball x/z (R)
 *     +0x0F2 (s16) y/height or timing offset for trajectory predictor (W)
 *     +0x0F8/+0x0FC (s16) predicted target x/z (R/W)
 *     +0x29A (u8)  match phase (1 = open play) (R)
 *     +0x2A8 (u8)  last-touch actor id (R)
 *     +0x2A9 (u8)  ball owner (0 = free) (R)
 *     +0x2B0 (u32) ball event code (R)
 *     +0x2B4 (s16) target/lock sentinel (0x7FFF = none) (R)
 *     +0x2CF (u8[2]) per-side possession flags (R)
 *     +0x2F4 (u8*) ptr to ball actor (R)
 *     +0x31C (s16) ball timer, frames since last touch (R)
 *     +0x31E (u16[2]) nearest/threatening opponent index table (R)
 *   Globals:
 *     D_800FF748 (u8[])  actor array (1000-byte stride)
 *     D_8010C32F (u8[])  player stat table +5 (side*264 + slot*12) (R)
 *     D_8010C330 (u32[]) player stat table +6 (side*264 + slot*12) (R)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x23E2 = 9186 raw (36.744 m) - deep half X threshold
 *   0x1990 = 6544 raw (26.176 m) - sweeper/midfield X threshold
 *   0x3320 = 13088 raw (52.352 m) - goal line X
 *   0x3360 = 13152 raw (52.608 m) - GK base anchor X on goal line
 *   0x1C00 = 7168 raw (28.672 m) - max forward advance from goal line when ball is upfield
 *   0x280  = 640 raw (2.56 m), 0x420 = 1056 raw (4.224 m) - goal-mouth arc X/Z radii
 *   0x10F4 = 4340 raw (17.36 m) - ball Z threshold for GK arc angle clamping
 *   0x23FFFFF = ~6144^2 (24.576 m)^2, 0x18FFFFF = ~5120^2 (20.48 m)^2, 0x3FFFFF = 2048^2 (8.192 m)^2
 *   0x1000 = 4096 raw (16.384 m), 0x400 = 1024 raw (4.096 m), 0x100 = 256 raw (1.024 m),
 *   0x80   = 128 raw (0.512 m), 0x40 = 64 raw (0.256 m)
 *   Reaction frames: 15 - stat/10 (event 1 or 3) or 23 - stat/10 (~0.5..0.77 s)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];
extern unsigned char D_8010C32F[];
extern unsigned char D_8010C330[];

extern s32 func_8001E5A4(unsigned int angle, int scale);
extern s32 func_8001E694(unsigned int angle, int scale);
extern void func_800479B8(unsigned char *actor, int mode);
extern void func_800484F8(int x, int z, unsigned int speed, int mode);
extern void func_800489A8(void);
extern void func_80048AD8(unsigned char *actor, unsigned int move_dir, unsigned int look_dir, unsigned int dist);
extern void func_80048D94(unsigned char *actor, unsigned int move_dir, unsigned int look_dir, unsigned int dist);
extern void func_80048FC4(unsigned char *actor, unsigned int move_dir, unsigned int look_dir, unsigned int dist);
extern unsigned int func_8004968C(unsigned char *actor);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);
extern void func_8005B908(unsigned char *actor);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005BD1C(unsigned char *actor);
extern unsigned int func_800B1128(int dist_sq);

void func_8006DD64(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *ball;
    unsigned int ball_angle;
    int react_frames;
    unsigned int event_code;
    int own_x;
    signed short anchor_x;
    signed short target_x, target_z;
    unsigned int target_angle;
    unsigned int dist;
    int dx, dz;

    B(actor, 0x3A7) = 1;
    B(actor, 0x3D1) = 1;
    func_8005BD1C(actor);

    ball_angle = func_8005B36C(S(actor, 2), S(actor, 6), S(scratch, 0x36), S(scratch, 0x66));

    event_code = W(scratch, 0x2B0);
    if (event_code == 1 || event_code == 3) {
        react_frames = 15 - (B(actor, 0x3B9) / 10);
    } else {
        react_frames = 23 - (B(actor, 0x3B9) / 10);
    }

    if (B(actor, 0x38D) == B(scratch, 0x2A8) || B(actor, 0x38D) == B(scratch, 0x2A9)) {
        B(actor, 0x396) = 0;
        B(actor, 0x397) = 0;
        func_8005B908(actor);
        return;
    }

    own_x = (B(actor, 0x399) != 0) ? -S(actor, 2) : S(actor, 2);
    if (B(scratch, 0x29A) != 1 ||
        (own_x < -0x23E2 && B(scratch, 0x2CF + B(actor, 0x398)) == 1 &&
         S(scratch, 0x31C) < react_frames && B(scratch, 0x2A9) == 0)) {
        unsigned int step_angle;
        func_8005BA30(actor, 0xE, 0xF);
        step_angle = func_8005B7CC(B(actor, 0x3AA), ball_angle & 0xFF, 0x38);
        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(step_angle & 0xFF, B(actor, 0x3AA), 0x10);
        if (W(actor, 0x3A0) < 0x13) {
            W(actor, 0x3A0) = 0;
        } else {
            W(actor, 0x3A0) -= 0x12;
        }
        func_8005B908(actor);
        return;
    }

    ball = (unsigned char *)W(scratch, 0x2F4);
    if (((B(actor, 0x399) != 0 && S(ball, 2) < 0) ||
         (B(actor, 0x399) == 0 && S(ball, 2) > 0)) &&
        own_x < -0x1990) {
        int abs_bx = S(ball, 2);
        signed short advance_x;
        unsigned int thresh;
        int diff;
        int close;

        if (abs_bx < 0) {
            abs_bx = -abs_bx;
        }
        advance_x = (signed short)((abs_bx * 7168) / 0x3320) - 0x3320;
        target_x = (B(actor, 0x399) != 0) ? (signed short)(-advance_x) : advance_x;

        target_angle = func_8005B36C(S(actor, 2), S(actor, 6), target_x, 0);
        dx = S(actor, 2) - target_x;
        dz = S(actor, 6);
        dist = func_800B1128(dx * dx + dz * dz) & 0xFC00;

        thresh = (H(actor, 0x38E) == 1) ? 0x400 : 0x100;
        if (dist < thresh) {
            if (H(actor, 0x38E) != 1) {
                func_8005BA30(actor, 1, 0);
            }
            B(actor, 0x3AA) = (unsigned char)func_8005B7CC(ball_angle & 0xFF, B(actor, 0x3AA), 0x10);
            B(actor, 0x3A4) = B(actor, 0x3AA);
            if (W(actor, 0x3A0) < 0x13) {
                W(actor, 0x3A0) = 0;
            } else {
                W(actor, 0x3A0) -= 0x12;
            }
            func_8005B908(actor);
            return;
        }

        diff = (int)((ball_angle - target_angle) & 0xFF);
        if (diff < 0x81) {
            close = (diff < 0x10);
        } else {
            close = ((int)((target_angle - ball_angle) & 0xFF) < 0x10);
        }
        if (close) {
            if (H(actor, 0x38E) != 5) {
                func_8005BA30(actor, 5, 0);
            }
            B(actor, 0x3AA) = (unsigned char)target_angle;
            B(actor, 0x3A4) = (unsigned char)target_angle;
            W(actor, 0x3A0) = 0x10;
            func_8005B908(actor);
            return;
        }

        func_80048AD8(actor, target_angle & 0xFF, ball_angle & 0xFF, dist & 0xFFFF);
        func_8005B908(actor);
        return;
    }

    if (W(scratch, 0x2B0) == 4 && S(actor, 0x3BC) != 0) {
        dx = S(actor, 2) - S(ball, 2);
        dz = S(actor, 6) - S(ball, 6);
        if (dx * dx + dz * dz <= 0x23FFFFF) {
            int stat_off = B(actor, 0x398) * 264 + B(actor, 0x392) * 12;
            int val = ((W(D_8010C330 + stat_off) >> 11) & 0x7F) + 0x64;
            int extra = B(D_8010C32F + stat_off) & 0xF;
            S(scratch, 0xF2) = (signed short)(((-val * 736) / 180) - extra + 0x46);
            func_800484F8(S(actor, 2), S(actor, 6), (unsigned short)(B(actor, 0x3B4) - 2), 2);
            target_x = S(scratch, 0xF8);
            target_z = S(scratch, 0xFC);
            goto move_to_target;
        }
    }

    anchor_x = (B(actor, 0x399) != 0) ? 0x3360 : -0x3360;
    if (S(scratch, 0x2B4) == 0x7FFF) {
        int abs_bz;
        target_angle = func_8005B36C(anchor_x, 0, S(scratch, 0x46), S(scratch, 0x76));
        abs_bz = S(scratch, 0x76);
        if (abs_bz < 0) {
            abs_bz = -abs_bz;
        }
        if (abs_bz >= 0x10F5) {
            unsigned int step = (unsigned int)((abs_bz - 0x10F4) / 80);
            if ((step & 0xFF) >= 0x20) {
                step = 0x20;
            }
            target_angle = func_8005B7CC((B(actor, 0x399) << 7) & 0x80, target_angle & 0xFF, step & 0xFF);
        }
    } else {
        S(scratch, 0xF2) = -0x2D0;
        func_800489A8();
        dx = S(actor, 2) - S(scratch, 0xF8);
        dz = S(actor, 6) - S(scratch, 0xFC);
        if (dx * dx + dz * dz <= 0x18FFFFF && func_8004968C(actor) <= 0x3FFFFF) {
            unsigned int idx = B(scratch, 0x31E + (B(actor, 0x399) ^ 1) * 2);
            unsigned char *threat = D_800FF748 + (idx % 24) * 1000;
            target_angle = func_8005B36C(anchor_x, 0, S(threat, 2), S(threat, 6));
        } else {
            target_angle = func_8005B36C(anchor_x, 0, S(scratch, 0xF8), S(scratch, 0xFC));
        }
    }

    target_angle = func_8005B7CC((target_angle + 4) & 0xF8, (B(actor, 0x399) << 7) & 0x80, 0x30) & 0xFF;
    target_x = (signed short)(anchor_x + func_8001E694(target_angle, 0x280));
    target_z = (signed short)func_8001E5A4(target_angle, 0x420);

move_to_target:
    dx = S(actor, 2) - target_x;
    dz = S(actor, 6) - target_z;
    dist = func_800B1128(dx * dx + dz * dz);
    target_angle = func_8005B36C(S(actor, 2), S(actor, 6), target_x, target_z);

    if (dist < 0x40) {
        func_8005BA30(actor, 0xE, 0xF);
        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(ball_angle & 0xFF, B(actor, 0x3AA), 0x10);
        B(actor, 0x3A4) = B(actor, 0x3AA);
        if (W(actor, 0x3A0) < 0x13) {
            W(actor, 0x3A0) = 0;
        } else {
            W(actor, 0x3A0) -= 0x12;
        }
        func_8005B908(actor);
        return;
    }

    if (dist < ((S(scratch, 0x2B4) == 0x7FFF) ? 0x100U : 0x80U)) {
        func_80048FC4(actor, target_angle & 0xFF, ball_angle & 0xFF, dist & 0xFFFF);
        func_8005B908(actor);
        return;
    }

    if (dist < 0x1000 && B(scratch, 0x2A9) != 0) {
        unsigned char ball_dir = B((unsigned char *)W(scratch, 0x2F4), 0x3A8);
        int diff = (int)((target_angle - ball_dir) & 0xFF);
        int aligned;
        if (diff < 0x81) {
            aligned = (diff < 0x21);
        } else {
            aligned = ((int)((ball_dir - target_angle) & 0xFF) < 0x21);
        }
        if (aligned) {
            func_80048AD8(actor, target_angle & 0xFF, ball_angle & 0xFF, dist & 0xFFFF);
            func_8005B908(actor);
            return;
        }
    }

    if (dist >= 0x400) {
        S(scratch, 0xF8) = target_x;
        S(scratch, 0xFC) = target_z;
        func_800479B8(actor, 1);
    } else {
        func_80048D94(actor, target_angle & 0xFF, ball_angle & 0xFF, dist & 0xFFFF);
    }
    func_8005B908(actor);
}
