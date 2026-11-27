/*
 * func_8003C46C - Diving header action state machine (substate 0..4).
 *
 * Purpose:
 *   Controls a player performing a diving header (animation 0x53 -> getup 0xF):
 *   - Substate 0: Initializes diving header check; aborts/switches to chase if ball is too low
 *                 or if ball timer == 1; otherwise falls through to substate 1.
 *   - Substate 1: Checks angular alignment to predicted ball (frame 5 = 0.167s) and current ball,
 *                 computes intercept velocity & target, and commits to diving header (substate 2).
 *   - Substate 2: Advances dive position, turns toward target heading, checks ball contact at
 *                 anim frame >= 5, and executes either a header shot or header pass/clearance.
 *   - Substate 3: Post-dive follow-through until animation resets to frame 0, then starts getup (0xF).
 *   - Substate 4: Plays getup animation until frame 0x2B (43 frames), then returns to state 0.
 *
 * Arguments:
 *   actor (u8*): player actor struct (1000 bytes).
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F800036 + i*2 (s16, R): predicted ball X at frame i (i=5 -> 0x40, i=8 -> 0x46).
 *   - 0x1F80004E + i*2 (s16, R): predicted ball Y at frame i (i=5 -> 0x58).
 *   - 0x1F800066 + i*2 (s16, R): predicted ball Z at frame i (i=5 -> 0x70).
 *   - 0x1F8000F0/0xF2/0xF4 (s16, R/W): target/contact xyz (0xF2 -= 0x80 for header height).
 *   - 0x1F8002B4/0x2B6 (s16, R): ball predicted landing coords passed to func_80049A94.
 *   - 0x1F8002F4 (u8*, R): pointer to ball actor.
 *   - 0x1F80031C (s16, R/W): ball timer (frames since last touch).
 *   - 0x1F80031E[2] (u8, R): GK actor IDs per attack direction (stride 2 bytes).
 *
 * Actor fields reads/writes:
 *   - actor: +0x02/+0x06 (s16 x/z, R/W), +0x26 (u16 rotation, W), +0x38D (u8 id),
 *            +0x390 (u8 anim frame), +0x392 (u8 team slot), +0x396 (u8 state, W),
 *            +0x397 (u8 substate, R/W), +0x398 (u8 side), +0x399 (u8 attack dir),
 *            +0x3AA (u8 body heading, R/W), +0x3AB (u8 target heading, R/W),
 *            +0x3AC (u8 mirror/foot, R/W), +0x3B1 (u8, W=0), +0x3B4 (u8 max speed),
 *            +0x3C2/+0x3C4/+0x3C6 (s16 shot target/params, W), +0x3CA/+0x3CE (s16 vx/vz step, R/W),
 *            +0x3D1 (u8, W).
 *   - ball:  +0x02/+0x06 (s16 x/z), +0x0C (s32 vy), +0x396 (u8 state, W=4),
 *            +0x3A4 (u8 move heading), +0x3B2 (u8, W=8), +0x3BC (s16, W=1),
 *            +0x3C8 (s16 spin/timer, W), +0x3CA (s16, W), +0x3DC (s16, R).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x80 (128) = 0.51 m vertical header offset; 0xC0 (192) = 0.77 m chase offset
 *   - 0x3321 (13089) = 52.36 m, 0x3320 (13088) = 52.35 m (goal line X)
 *   - 0x3301 (13057) = 52.23 m (predicted frame-8 X limit)
 *   - 0x1001 (4097) = 16.39 m (shooting range X threshold)
 *   - 0x24000 (147456 = 384^2 -> 1.54 m), 0x9000 (36864 = 192^2 -> 0.77 m)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_8010C330[];

extern void func_8005BD1C(unsigned char *actor);
extern void func_8005C2AC(unsigned char *actor, int anim, int frame, int heading, int mirror);
extern void func_8005B908(unsigned char *actor);
extern int func_8001E694(int angle, int scale);
extern int func_8001E5A4(int angle, int scale);
extern void func_80038F18(unsigned char *actor, int vx, int vz, int a3);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_800434D4(unsigned char *actor, int ang);
extern int func_80053EB8(unsigned char *actor);
extern int func_80033B88(unsigned char *actor);
extern int func_8005B7CC(int target, int cur, int step);
extern unsigned int func_800B1128(int val);
extern int func_80052A68(unsigned char *actor, int frame_idx, int dist_sq);
extern int func_8003CEE4(unsigned char *actor, int frame_idx, int a2, int a3, int dist_sq);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern int func_8003D274(unsigned char *actor, int a1, int a2);
extern int func_80034734(unsigned char *actor);
extern int func_8001E6B8(int n);
extern void func_80034ACC(unsigned char *actor);
extern int func_8005C7A0(int n);
extern void func_8004B0F0(unsigned char *actor, int heading, int power, int lift);
extern void func_800AB620(int sfx);
extern void func_8004B278(void);
extern void func_80049A94(unsigned char *actor, int a1, int a2, int x, int z);
extern void func_80056A98(unsigned char *actor);
extern void func_800498AC(unsigned char *actor);

void func_8003C46C(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *ball;

    switch (B(actor, 0x397)) {
    case 0:
    {
        signed short y;
        func_8005BD1C(actor);
        B(actor, 0x3D1) = 1;
        func_8005C2AC(actor, 0x53, 0, B(actor, 0x3AA), B(actor, 0x3AC));
        y = H(scratch, 0xF2) - 0x80;
        S(scratch, 0xF2) = y;
        if (S(scratch, 0x31C) == 1) {
            B(actor, 0x396) = 1;
            func_8005B908(actor);
            B(actor, 0x397) = 0;
            break;
        }
        if (S(scratch, 0x58) < y) {
            int vx_off, vz_off, ang;
            ball = (unsigned char *)W(scratch, 0x2F4);
            vx_off = (signed short)func_8001E694(B(ball, 0x3A4), 0xC0);
            vz_off = (signed short)func_8001E5A4(B(ball, 0x3A4), 0xC0);
            func_80038F18(actor, vx_off, vz_off, 1);
            func_8005B908(actor);
            ball = (unsigned char *)W(scratch, 0x2F4);
            ang = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));
            if (func_800434D4(actor, ang & 0xFF) != 0) {
                break;
            }
            if ((func_80053EB8(actor) & 0xFF) == 0 ||
                S((unsigned char *)W(scratch, 0x2F4), 0x3DC) == 0) {
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 0;
            }
            break;
        }
        /* Fall through to case 1 */
    }
    case 1:
    {
        int ang_pred, ang_ball;
        int pred_x, in_goal_area, ok;
        int aim_h, mirror;

        ang_pred = func_8005B36C(S(actor, 2), S(actor, 6), S(scratch, 0x40), S(scratch, 0x70));
        ball = (unsigned char *)W(scratch, 0x2F4);
        ang_ball = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));

        pred_x = S(scratch, 0x40);
        in_goal_area = ((B(actor, 0x399) != 0) ? (-pred_x >= 0x3321) : (pred_x >= 0x3321));
        ok = 1;
        if (in_goal_area) {
            unsigned char b_h = B(ball, 0x3A4);
            unsigned int d = (b_h - ang_ball) & 0xFF;
            if (d < 0x81) {
                if (d >= 0x60) ok = 0;
            } else {
                if (((ang_ball - b_h) & 0xFF) >= 0x60) ok = 0;
            }
        }

        if (ok) {
            unsigned char body_h = B(actor, 0x3AA);
            unsigned int d1 = (ang_pred - body_h) & 0xFF;
            unsigned int d2;
            if (d1 < 0x81) {
                if (d1 >= 0x31) ok = 0;
            } else {
                if (((body_h - ang_pred) & 0xFF) >= 0x31) ok = 0;
            }
            if (ok) {
                d2 = (ang_ball - ang_pred) & 0xFF;
                if (d2 < 0x81) {
                    if (d2 >= 0x50) ok = 0;
                } else {
                    if (((ang_pred - ang_ball) & 0xFF) >= 0x50) ok = 0;
                }
            }
        }

        if (!ok) {
            if (B(actor, 0x38D) == B(scratch, 0x31E + (B(actor, 0x399) << 1))) {
                unsigned int stat = W(D_8010C330, B(actor, 0x398) * 264 + B(actor, 0x392) * 12);
                int h_off = -((int)((((stat >> 11) & 0x7F) + 0x64) << 8)) / 180;
                S(scratch, 0xF2) = (signed short)h_off;
                func_80038F18(actor, 0, 0, 0);
            } else {
                B(actor, 0x396) = 0xE;
                B(actor, 0x397) = 0x50;
            }
            func_8005B908(actor);
            break;
        }

        {
            int fwd_x = (B(actor, 0x399) != 0) ? -S(actor, 2) : S(actor, 2);
            if (fwd_x >= 0x1001) {
                int gx = (B(actor, 0x399) != 0) ? -0x3320 : 0x3320;
                aim_h = func_8005B36C(S(actor, 2), S(actor, 6), gx, 0);
            } else {
                aim_h = func_80033B88(actor);
            }
        }

        {
            unsigned char body_h = B(actor, 0x3AA);
            unsigned int d = (aim_h - body_h) & 0xFF;
            int close = (d < 0x81) ? (d < 0x11) : (((body_h - aim_h) & 0xFF) < 0x11);
            if (close) {
                unsigned int stat = W(D_8010C330, B(actor, 0x398) * 264 + B(actor, 0x392) * 12);
                mirror = (stat >> 4) & 1;
            } else {
                ball = (unsigned char *)W(scratch, 0x2F4);
                mirror = ((aim_h - B(ball, 0x3A4)) & 0xFF) < 0x81;
            }
        }

        aim_h = func_8005B7CC(aim_h & 0xFF, B(actor, 0x3AA), 0x10);
        func_8005C2AC(actor, 0x53, 0, aim_h & 0xFF, mirror);

        {
            int frame_idx = 5;
            int step_frames = frame_idx + 1;
            int dir = func_8005B36C(S(scratch, 0xF0), S(scratch, 0xF4),
                                    S(scratch, 0x36 + frame_idx * 2), S(scratch, 0x66 + frame_idx * 2));
            int dx = S(scratch, 0xF0) - S(scratch, 0x36 + frame_idx * 2);
            int dz = S(scratch, 0xF4) - S(scratch, 0x66 + frame_idx * 2);
            unsigned int spd = func_800B1128(dx * dx + dz * dz) / (unsigned int)step_frames;

            if (B(actor, 0x3B4) < spd) {
                spd = B(actor, 0x3B4);
            }
            S(actor, 0x3CA) = func_8001E694(dir & 0xFF, spd);
            S(actor, 0x3CE) = func_8001E5A4(dir & 0xFF, spd);
            S(scratch, 0xF0) += S(actor, 0x3CA) * step_frames;
            S(scratch, 0xF4) += S(actor, 0x3CE) * step_frames;

            if ((func_80052A68(actor, frame_idx, 0x24000) & 0xFF) == 0 &&
                (func_8003CEE4(actor, frame_idx, 1, 0, 0x9000) & 0xFF) != 0) {
                unsigned char heading;
                int turn;

                func_8005BA30(actor, 0x53, 0);
                B(actor, 0x3D1) = 3;
                B(actor, 0x3AC) = (unsigned char)mirror;
                heading = (unsigned char)func_8005B7CC(aim_h & 0xFF, B(actor, 0x3AA), 0xC);
                B(actor, 0x3AA) = heading;
                turn = 0x1C0 - heading;
                H(actor, 0x26) = (unsigned short)((turn % 256) << 4);
                B(actor, 0x397) = 2;
                B(actor, 0x3AB) = (unsigned char)aim_h;
                H(actor, 2) += H(actor, 0x3CA);
                H(actor, 6) += H(actor, 0x3CE);
                break;
            }
        }

        ball = (unsigned char *)W(scratch, 0x2F4);
        {
            int vy = SW(ball, 0xC);
            int abs_x8 = S(scratch, 0x46);
            if (vy < 0) vy = -vy;
            if (abs_x8 < 0) abs_x8 = -abs_x8;

            if (vy < 0xD || abs_x8 >= 0x3301) {
                B(actor, 0x396) = 0xE;
                B(actor, 0x397) = 0x50;
            } else {
                int vx_off = (signed short)func_8001E694(B(ball, 0x3A4), 0xC0);
                int vz_off = (signed short)func_8001E5A4(B(ball, 0x3A4), 0xC0);
                func_80038F18(actor, vx_off, vz_off, 1);
            }
            func_8005B908(actor);
        }
        break;
    }

    case 2:
    {
        unsigned char heading;
        int turn;

        func_8005BD1C(actor);
        heading = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
        B(actor, 0x3AA) = heading;
        turn = 0x1C0 - heading;
        H(actor, 0x26) = (unsigned short)((turn % 256) << 4);
        H(actor, 2) += H(actor, 0x3CA);
        H(actor, 6) += H(actor, 0x3CE);

        if (B(actor, 0x390) < 5) {
            break;
        }
        B(actor, 0x397)++;
        if ((func_8003D274(actor, 1, 1) & 0xFF) != 0) {
            int gx = (B(actor, 0x399) != 0) ? -0x3320 : 0x3320;
            int aim_h = func_8005B36C(S(actor, 2), S(actor, 6), gx, 0);
            int fwd_x = (B(actor, 0x399) != 0) ? -S(actor, 2) : S(actor, 2);
            int shoot = 0;

            if (fwd_x >= 0x1001) {
                unsigned char body_h = B(actor, 0x3AA);
                unsigned int d = (aim_h - body_h) & 0xFF;
                shoot = (d < 0x81) ? (d < 0x41) : (((body_h - aim_h) & 0xFF) < 0x41);
            }

            if (shoot) {
                S(actor, 0x3C2) = func_80034734(actor) & 0xFF;
                S(actor, 0x3C4) = (func_8001E6B8(4) == 0) ? 1 : 0;
                S(actor, 0x3C6) = 0;
                B(actor, 0x3B1) = 0;
                func_80034ACC(actor);
            } else {
                int pass_h = func_80033B88(actor);
                int pwr, lift;
                aim_h = func_8005B7CC(pass_h & 0xFF, B(actor, 0x3AA), 0x34);
                pwr = (signed short)func_8005C7A0(8) + 0x70;
                lift = (signed short)func_8005C7A0(4) + 0x1C;
                func_8004B0F0(actor, aim_h & 0xFF, pwr, lift);
            }

            ball = (unsigned char *)W(scratch, 0x2F4);
            S(ball, 0x3CA) = ((aim_h - B(actor, 0x3AA)) & 0xFF) < 0x81;

            {
                int r = func_8001E6B8(3);
                unsigned char body_h = B(actor, 0x3AA);
                unsigned int d = (body_h - aim_h) & 0xFF;
                int spin_adj = (d < 0x81) ? ((int)d >> 6) : ((int)((aim_h - body_h) & 0xFF) >> 6);
                S(ball, 0x3C8) = (9 - r) - spin_adj;
            }

            B(actor, 0x3AB) = (unsigned char)aim_h;
            B(ball, 0x396) = 4;
            S(ball, 0x3BC) = 1;
            B(ball, 0x3B2) = 8;
            func_800AB620(0x507);
            func_8004B278();
            S(scratch, 0x31C) = (B(actor, 0x399) ^ 1) & 0xFF;
            func_80049A94(actor, 1, 1, S(scratch, 0x2B4), S(scratch, 0x2B6));
            func_80056A98(actor);
        }
        break;
    }

    case 3:
    {
        unsigned char heading;
        int turn;

        func_8005BD1C(actor);
        heading = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 4);
        B(actor, 0x3AA) = heading;
        turn = 0x1C0 - heading;
        H(actor, 0x26) = (unsigned short)((turn % 256) << 4);
        if (B(actor, 0x390) == 0) {
            func_8005BA30(actor, 0xF, 0x27);
            B(actor, 0x397)++;
        }
        break;
    }

    case 4:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0x2B) {
            B(actor, 0x396) = 0;
            B(actor, 0x397) = 0;
        }
        break;
    }

    func_800498AC(actor);
}
