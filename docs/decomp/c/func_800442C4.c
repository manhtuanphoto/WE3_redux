/*
 * func_800442C4 - Secondary defender jockey / contain / tackle state handler (substates 0..4).
 *
 * Purpose:
 *   Controls the secondary pressing/containing defender (scratch 0x31F[attack_dir]) while
 *   press button (0x26) is held in open play (0x29A == 1, 0x2FC == 0):
 *   - Entry guards: switches to state 0x1B/1 if actor owns ball (0x2A9); exits to state 0/1
 *     if press button released, not open play, frozen, or actor != scratch 0x31F[attack_dir].
 *   - Computes jockey target (target_x, target_z) ahead of the ball or ball-owner toward
 *     own goal (+-0x3320, 0), clamping X within +-0x25E2 (38.79 m) if marked opponent is
 *     near own goal, and checks func_800551C8 for state 0xF transition.
 *   - Substate 0: Advances substate to 1 and falls through into Substate 1.
 *   - Substate 1: Calls func_80049F90 if ball timer < 8; stops to substate 2 (0x3B0 = 4) if
 *                 within 0x80 (0.51 m) of jockey point; otherwise steers via func_80048D94
 *                 and advances to substate 3 when aligned within 0x51 (~114 deg) of ball heading.
 *   - Substate 2: Decelerates (speed -= 0x12) while turning toward ball (0x3AA) for 0x3B0 frames,
 *                 then returns to substate 1 once distance >= 0x101 (1.03 m) and timer >= 8.
 *   - Substate 3: Sets scratch 0xF8/0xFC and calls func_800479B8(actor, 1); if within 0x200
 *                 (2.05 m) and not aligned in safe zone (< 0x3220), launches lunge/tackle
 *                 (anim 0x26, substate 4, 0x3D0 = 0xA, speed = 0x30).
 *   - Substate 4: Slides/lunges via func_80048D94 until 0x3D0 expires or X >= 0x3220, then
 *                 returns to substate 1.
 *   - Epilogue: Calls func_800434D4(actor, ball_ang) (unless skipped) and func_8005B908(actor).
 *
 * Arguments:
 *   actor (u8*): player actor struct (1000 bytes).
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F800026[2] (u16, R): per-side press/contain input bitmask (tested against +0x3D4).
 *   - 0x1F800036 (s16, R): predicted ball X at frame 0.
 *   - 0x1F80003C/0x6C (s16, R): predicted ball X/Z at frame 3 (0.10 s).
 *   - 0x1F80003E/0x6E (s16, R): predicted ball X/Z at frame 4 (0.13 s).
 *   - 0x1F800046/0x76 (s16, R): predicted ball X/Z at frame 8 (0.27 s).
 *   - 0x1F8000F8/0xFC (s16, R/W): intercept target X/Z.
 *   - 0x1F8000FE/0x100 (s16, R): ball bounce/deflection target X/Z.
 *   - 0x1F80029A (u8, R): match phase (1 = open play).
 *   - 0x1F8002A9 (u8, R): ball owner actor ID (0 = free).
 *   - 0x1F8002B0 (u32, R): ball event code (1 = shot/pass bounce).
 *   - 0x1F8002F4 (u8*, R): pointer to ball actor.
 *   - 0x1F8002FC (u8, R): freeze/replay flag.
 *   - 0x1F80031C (s16, R): ball timer (frames since last touch).
 *   - 0x1F80031F[2] (u8, R): secondary presser actor ID (stride 2 by attack dir).
 *
 * Actor fields reads/writes:
 *   - actor:  +0x02/+0x06 (s16 x/z, R), +0x38D (u8 id, R), +0x38E (u16 anim id, R),
 *             +0x390 (u8 anim frame, R), +0x396 (u8 state, W), +0x397 (u8 substate, R/W),
 *             +0x398 (u8 side, R), +0x399 (u8 attack dir, R), +0x39A (u8 role/GK flag, R),
 *             +0x3A0 (u32 speed, R/W), +0x3A4 (u8 move heading, R), +0x3A7 (u8, W=1),
 *             +0x3AA (u8 body heading, R/W), +0x3AE (u16 timer, R/W),
 *             +0x3B0 (u8 pause timer, R/W), +0x3D0 (u8 tackle timer, R/W),
 *             +0x3D1 (u8, W=2), +0x3D4 (u16 input mask, R).
 *   - marked: +0x02/+0x06 (s16 x/z, R).
 *   - ball:   +0x3A8 (u8 heading, R), +0x3CC (s16 bounce count, R).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x3320 (13088) = 52.35 m (goal line X); 0x3220 (12832) = 51.33 m (near endline X).
 *   - 0x25E2 (9698) = 38.79 m (box clamp X); -0x23E2 (-9186) = -36.74 m (penalty box X).
 *   - 0xCB6 (3254) = 13.02 m (goal-box Z band); 0x18FFFF (~1280^2) -> 5.12 m from goal.
 *   - 0x800 (2048) = 8.19 m jockey offset; 0x300 (768) = 3.07 m; 0x200 (512) = 2.05 m.
 *   - 0x80 (128) = 0.51 m arrival threshold; 0x101 (257) = 1.03 m resume threshold.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];

extern void func_8005BD1C(unsigned char *actor);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005BA78(unsigned char *actor, int anim, int frame);
extern int func_8001E694(int angle, int scale);
extern int func_8001E5A4(int angle, int scale);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_800551C8(unsigned char *actor);
extern int func_8005CE14(int actor_id);
extern unsigned int func_800B1128(int val);
extern void func_80049F90(unsigned char *actor);
extern void func_80048D94(unsigned char *actor, int target_ang, int ball_ang, int dist);
extern int func_8005B7CC(int target, int cur, int step);
extern void func_800479B8(unsigned char *actor, int mode);
extern void func_800434D4(unsigned char *actor, int ball_ang);
extern void func_8005B908(unsigned char *actor);

void func_800442C4(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *ball;
    unsigned char id;
    int target_x, target_z;
    int recent_touch = 0;
    int ball_ang, target_ang;
    unsigned int dist;
    int dx, dz;

    func_8005BD1C(actor);

    id = B(actor, 0x38D);
    if (id == B(scratch, 0x2A9)) {
        B(actor, 0x396) = 0x1B;
        B(actor, 0x397) = 1;
        func_8005B908(actor);
        return;
    }

    if ((H(scratch, 0x26 + B(actor, 0x398) * 2) & H(actor, 0x3D4)) == 0 ||
        B(scratch, 0x29A) != 1 ||
        B(scratch, 0x2FC) != 0) {
        B(actor, 0x396) = (H(actor, 0x38E) == 1) ? 0 : 1;
        B(actor, 0x397) = 0;
        func_8005B908(actor);
        return;
    }

    if (id != B(scratch, 0x31F + B(actor, 0x399) * 2)) {
        B(actor, 0x396) = 1;
        B(actor, 0x397) = 0;
        func_8005B908(actor);
        return;
    }

    B(actor, 0x3A7) = 1;
    B(actor, 0x3D1) = 2;
    H(actor, 0x3AE) = (unsigned short)(H(actor, 0x3AE) - 1);

    ball = (unsigned char *)W(scratch, 0x2F4);
    if (B(scratch, 0x2A9) != 0) {
        unsigned char owner_ang = B(ball, 0x3A8);
        target_x = H(scratch, 0x3C) + func_8001E694(owner_ang, 0x800);
        target_z = H(scratch, 0x6C) + func_8001E5A4(owner_ang, 0x800);
        recent_touch = 0;
    } else {
        int goal_x, goal_ang, offset;
        recent_touch = (S(scratch, 0x31C) < 8) ? 1 : 0;

        if (W(scratch, 0x2B0) == 1 && S(ball, 0x3CC) >= 2) {
            target_x = H(scratch, 0xFE);
            target_z = H(scratch, 0x100);
        } else {
            target_x = H(scratch, 0x46);
            target_z = H(scratch, 0x76);
        }

        goal_x = (B(actor, 0x399) != 0) ? 0x3320 : -0x3320;
        goal_ang = func_8005B36C((signed short)target_x, (signed short)target_z, goal_x, 0);

        if (B(actor, 0x399) != 0) {
            offset = (-(signed short)target_x < -0x23E2) ? 0x300 : 0x800;
        } else {
            offset = ((signed short)target_x < -0x23E2) ? 0x300 : 0x800;
        }

        target_x += func_8001E694(goal_ang & 0xFF, offset);
        target_z += func_8001E5A4(goal_ang & 0xFF, offset);
    }

    if ((recent_touch & 0xFF) == 0 && func_800551C8(actor) != 0) {
        B(actor, 0x396) = 0xF;
        B(actor, 0x397) = 0;
        func_8005B908(actor);
        return;
    }

    {
        int mark_id = func_8005CE14(B(actor, 0x38D)) & 0xFF;
        unsigned char *marked = D_800FF748 + (mark_id % 24) * 1000;

        if (B(scratch, 0x2A9) != 0) {
            int abs_tz = (signed short)target_z;
            if (abs_tz < 0) {
                abs_tz = -abs_tz;
            }
            if (abs_tz < 0xCB6) {
                int mx = S(marked, 2);
                int mz = S(marked, 6);
                int gx = (B(actor, 0x399) != 0) ? (mx - 0x3320) : (mx + 0x3320);

                if (gx * gx + mz * mz <= 0x18FFFF) {
                    if ((signed short)target_x >= 0x25E3) {
                        signed short bx = S(scratch, 0x36);
                        target_x = (bx < 0x25E2) ? 0x25E2 : (int)bx;
                    }
                    if ((signed short)target_x < -0x25E2) {
                        signed short bx = S(scratch, 0x36);
                        target_x = (bx < -0x25E2) ? (int)bx : -0x25E2;
                    }
                }
            }
        }
    }

    ball_ang = func_8005B36C(S(actor, 2), S(actor, 6), S(scratch, 0x3E), S(scratch, 0x6E));
    target_ang = func_8005B36C(S(actor, 2), S(actor, 6), (signed short)target_x, (signed short)target_z);
    dx = S(actor, 2) - (signed short)target_x;
    dz = S(actor, 6) - (signed short)target_z;
    dist = func_800B1128(dx * dx + dz * dz) & 0xFFC0;

    switch (B(actor, 0x397)) {
    case 0:
        B(actor, 0x397) = B(actor, 0x397) + 1;
        /* fall through */

    case 1:
        if ((recent_touch & 0xFF) != 0) {
            if (S(scratch, 0x31C) >= 2) {
                int f8_ang = func_8005B36C(S(actor, 2), S(actor, 6), S(scratch, 0xF8), S(scratch, 0xFC));
                int d = (f8_ang - B(actor, 0x3AA)) & 0xFF;
                int aligned = (d < 0x81)
                    ? (d < 0x40)
                    : (((B(actor, 0x3AA) - f8_ang) & 0xFF) < 0x40);
                if (!aligned && H(actor, 0x38E) != 1) {
                    func_8005BA30(actor, 1, 0);
                }
            }
            func_80049F90(actor);
        } else if (dist < 0x80) {
            if (H(actor, 0x38E) != 1) {
                func_8005BA30(actor, 1, 0);
            }
            B(actor, 0x3B0) = 4;
            B(actor, 0x397) = 2;
        } else {
            func_80048D94(actor, target_ang & 0xFF, ball_ang & 0xFF, (int)dist);
            if (B(actor, 0x39A) == 0 || B(scratch, 0x328) != 0) {
                if (dist > 0x400000) {
                    B(actor, 0x397) = 3;
                } else {
                    unsigned char *b = (unsigned char *)W(scratch, 0x2F4);
                    unsigned char b_ang = B(b, 0x3A8);
                    int d = (ball_ang - b_ang) & 0xFF;
                    int aligned = (d < 0x81)
                        ? (d < 0x51)
                        : (((b_ang - ball_ang) & 0xFF) < 0x51);
                    if (aligned) {
                        B(actor, 0x397) = 3;
                    }
                }
            }
        }
        break;

    case 2:
    {
        unsigned int spd;
        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(ball_ang & 0xFF, B(actor, 0x3AA), 0x10);
        spd = W(actor, 0x3A0);
        W(actor, 0x3A0) = (spd < 0x13) ? 0 : (spd - 0x12);

        if (B(actor, 0x3B0) != 0) {
            B(actor, 0x3B0) = B(actor, 0x3B0) - 1;
        } else if (dist >= 0x101 && (recent_touch & 0xFF) == 0) {
            B(actor, 0x397) = 1;
        }
        break;
    }

    case 3:
    {
        int do_tackle = 1;
        H(actor, 0x3AE) = (unsigned short)(H(actor, 0x3AE) - 2);

        if (B(scratch, 0x2A9) != 0) {
            int dir_ang = (B(actor, 0x399) << 7) & 0xFF;
            int d = (target_ang - dir_ang) & 0xFF;
            int in_front = (d < 0x81)
                ? (d < 0x70)
                : (((dir_ang - target_ang) & 0xFF) < 0x70);

            S(scratch, 0xF8) = (signed short)target_x;
            if (!in_front) {
                if ((signed short)target_z > 0) {
                    target_z -= 0x200;
                } else {
                    target_z += 0x200;
                }
            }
            S(scratch, 0xFC) = (signed short)target_z;
        }

        func_800479B8(actor, 1);

        if (dist >= 0x200) {
            do_tackle = 0;
        } else {
            unsigned char *b = (unsigned char *)W(scratch, 0x2F4);
            unsigned char b_ang = B(b, 0x3A8);
            int d1 = (ball_ang - b_ang) & 0xFF;
            int cond1 = (d1 < 0x81) ? (d1 < 0x68) : (((b_ang - ball_ang) & 0xFF) < 0x68);
            if (cond1) {
                int d2 = (ball_ang - B(actor, 0x3AA)) & 0xFF;
                int cond2 = (d2 < 0x81) ? (d2 < 0x58) : (((B(actor, 0x3AA) - ball_ang) & 0xFF) < 0x58);
                if (cond2) {
                    int safe_x = (B(actor, 0x399) != 0)
                        ? (-S(actor, 2) < 0x3220)
                        : (S(actor, 2) < 0x3220);
                    if (safe_x) {
                        do_tackle = 0;
                    }
                }
            }
        }

        if (do_tackle) {
            func_8005BA78(actor, 0x26, B(actor, 0x390));
            B(actor, 0x397) = 4;
            B(actor, 0x3D0) = 0xA;
            W(actor, 0x3A0) = 0x30;
        }
        break;
    }

    case 4:
        func_80048D94(actor, B(actor, 0x3A4), ball_ang & 0xFF, 0xFF);
        if (B(actor, 0x3D0) != 0) {
            int safe_x = (B(actor, 0x399) != 0)
                ? (-S(actor, 2) < 0x3220)
                : (S(actor, 2) < 0x3220);
            if (safe_x) {
                break;
            }
        }
        B(actor, 0x397) = 1;
        break;
    }

    func_800434D4(actor, ball_ang & 0xFF);
    func_8005B908(actor);
}
