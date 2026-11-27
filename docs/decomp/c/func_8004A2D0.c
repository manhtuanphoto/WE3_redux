/*
 * func_8004A2D0
 *
 * Purpose:
 *   Per-frame ball physics, carrier attachment, and match-boundary step.
 *   Updates goalkeeper/air stoppage flag (+0x2FC), saves previous ball xyz into
 *   +0x3C2..+0x3C6, integrates ball motion/spin/gravity/bounce/grass friction/
 *   quadratic air drag across ball states 0..4, computes ball-to-target-goal
 *   heading (+0x3A8), and triggers pitch/goal-line/play-phase handlers.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Scratchpad (0x1F800000) fields read:
 *   - +0x36 (s16), +0x66 (s16): predicted ball target x/z
 *   - +0xE0 (u16): stoppage/set-piece active flag
 *   - +0x290 (u32): frame counter
 *   - +0x29A (u8): match phase (1 = open play, 6/8 = special phases)
 *   - +0x2A8 (u8): last-touch actor ID
 *   - +0x2AA (u8): ball owner / previous owner ID
 *   - +0x2AC (u8): previous owner ID (used for attack direction lookup)
 *   - +0x2F4 (ptr): pointer to ball actor struct
 *   - +0x332 (u8): GK contact/hold counter
 *
 * Scratchpad / global fields written:
 *   - +0x104..+0x154: matrix/vector scratch workspace for carrier ball attachment
 *   - +0x2A9 (u8), +0x2AC (u8), +0x2B0 (u32), +0x2B4 (s16), +0x2B8 (s16),
 *     +0x2CF[side] (u8), +0x2F2 (u8), +0x2FC (u8)
 *   - D_8010A396 (u8): set to 0xFF when ball is carried (state 3)
 *
 * Ball actor fields read/written:
 *   - +0x02/+0x04/+0x06 (s16): ball x/y/z (+0x04 negative = up, ground = -0x20)
 *   - +0x0C (s32): vertical velocity (<< 8)
 *   - +0x26 (u16): yaw rotation, +0x28 (u16): roll spin angle
 *   - +0x396 (u8): ball state (0=stopped, 1=air/bounce, 2=rolling, 3=carried, 4=settle)
 *   - +0x3A0 (u32): horizontal speed (<< 8)
 *   - +0x3A4 (u8): move heading, +0x3A8 (u8): heading from predicted pos to goal
 *   - +0x3B0 (u8), +0x3B1 (u8): carrier blend counter (0..32), +0x3B2 (u8)
 *   - +0x3BC (s16): surface/bounce restitution profile (0..3)
 *   - +0x3C2/+0x3C4/+0x3C6 (s16): previous frame ball x/y/z
 *   - +0x3C8 (s16): curve/swerve interval (frames), +0x3CA (s16): swerve sign
 *   - +0x3CC (s16): timer decremented toward 1
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   - -0x20 (-32) -> -0.128 m height (ball radius resting on pitch)
 *   - -0x74 (-116) -> -0.464 m height (GK high-ball flag threshold)
 *   - -0x5BF (-1471) -> -5.884 m/frame<<8 vertical bounce cutoff to settle into roll
 *   - -0x44FF (-17663) -> strong bounce threshold resetting surface profile +0x3BC
 *   - 0x2E0 (736), 0x2C0 (704), 0x320 (800), 0x338 (824) -> gravity per frame (<< 8)
 *   - 14680064 (7 << 21) -> quadratic air-drag / rolling-drag divisor
 *   - 0x3001 (12289) -> fast roll speed threshold (0x3000 = 48 raw/frame = 5.76 m/s) for grass bob
 *   - 0x100 (256) -> 1.0 raw/frame (0.12 m/s) rolling stop threshold
 *   - 0x3320 (13088) -> 52.352 m goal line x-coordinate
 *   - 0x3361 (13153) -> 52.612 m out-of-bounds / goal-line crossing threshold
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];

extern int func_8001E5A4(unsigned int angle, int scale);
extern int func_8001E694(unsigned int angle, int scale);
extern int func_8001E6B8(int n);
extern void func_8004B414(void);
extern void func_8004C0B0(void);
extern unsigned char *func_8004E6D4(unsigned int anim_id, unsigned int anim_frame);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern void func_800989B4(unsigned char *ball);
extern void func_800A7450(void);
extern int func_800A8B24(void);
extern void func_800A8B94(unsigned char *ball);
extern void func_800A8F3C(void);
extern int func_800B1128(int val);
extern void func_800B12C8(unsigned char *mat, unsigned char *in_vec, unsigned char *out_vec);
extern void func_800B1318(unsigned char *mat, unsigned char *trans);
extern void func_800B1348(unsigned char *mat, unsigned char *scale);
extern void func_800B1488(unsigned char *rot, unsigned char *mat);

void func_8004A2D0(void)
{
    unsigned char *actor;
    unsigned char *ball;
    unsigned char actor_id;
    int timer;
    int vy;
    int abs_vy;
    unsigned int spd;
    int interval;
    unsigned int ratio;
    int denom;
    int speed_3d;
    int goal_x;
    unsigned char phase;

    actor = D_800FF748 + (B(Scratchpad, 0x2A8) % 24) * 1000;
    actor_id = B(actor, 0x38D);
    if ((actor_id == 1 || actor_id == 12) &&
        (B(Scratchpad, 0x332) < 4 || S((unsigned char *)W(Scratchpad, 0x2F4), 4) < -0x74)) {
        B(Scratchpad, 0x2FC) = 1;
    } else {
        B(Scratchpad, 0x2FC) = 0;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    timer = S(ball, 0x3CC);
    S(ball, 0x3CC) = (timer < 2) ? 1 : (timer - 1);

    B(Scratchpad, 0x2AC) = B(Scratchpad, 0x2AA);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    H(ball, 0x3C2) = H(ball, 2);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    H(ball, 0x3C4) = H(ball, 4);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    H(ball, 0x3C6) = H(ball, 6);

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    switch (B(ball, 0x396)) {
    case 0:
        W(ball, 0x3A0) = 0;
        SW((unsigned char *)W(Scratchpad, 0x2F4), 0xC) = 0;
        break;

    case 4:
        if (SW(ball, 0xC) == 0 && S(ball, 4) == -0x20) {
            B(ball, 0x396) = 2;
        } else {
            B(ball, 0x396) = 1;
        }
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        vy = SW(ball, 0xC);
        abs_vy = (vy < 0) ? -vy : vy;
        H(ball, 0x28) -= (W(ball, 0x3A0) + (unsigned int)abs_vy) >> 5;
        break;

    case 1:
        interval = S(ball, 0x3C8);
        if (interval != 0 && (SW(Scratchpad, 0x290) % interval) == 0) {
            if (S(ball, 0x3CA) != 0) {
                B(ball, 0x3A4) += 1;
            } else {
                B(ball, 0x3A4) -= 1;
            }
        }

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        S(ball, 2) += func_8001E694(B(ball, 0x3A4), SW(ball, 0x3A0)) >> 8;

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        S(ball, 4) += SW(ball, 0xC) >> 8;

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        S(ball, 6) += func_8001E5A4(B(ball, 0x3A4), SW(ball, 0x3A0)) >> 8;

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        H(ball, 0x26) = (0x80 - B(ball, 0x3A4)) << 4;

        vy = SW(ball, 0xC);
        abs_vy = (vy < 0) ? -vy : vy;
        H(ball, 0x28) -= (W(ball, 0x3A0) + (unsigned int)abs_vy) >> 5;

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        if (S(ball, 4) >= -0x20) {
            /* Ground bounce */
            H(Scratchpad, 0x2B8) = 0x7FFF;
            H(Scratchpad, 0x2B4) = 0x7FFF;

            vy = SW(ball, 0xC);
            spd = W(ball, 0x3A0);
            denom = vy + 1;
            if (denom < 0) {
                denom = -denom;
            }
            ratio = spd / (unsigned int)denom;
            if ((int)ratio >= 9) {
                ratio = 8;
            }

            switch (S(ball, 0x3BC)) {
            case 0:
                W(ball, 0x3A0) = spd - ((spd * (0x40U - ratio * 4U)) >> 8);
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                vy = SW(ball, 0xC);
                SW(ball, 0xC) = vy - ((vy * (int)(0x60 - ratio * 4)) / 256);
                break;

            case 1:
                W(ball, 0x3A0) = spd - ((spd * (0x40U - ratio * 4U)) >> 7);
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                vy = SW(ball, 0xC);
                SW(ball, 0xC) = vy - ((vy * (int)(0x40 - ratio * 4)) / 240);
                break;

            case 2:
                S(ball, 0x3BC) = 0;
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                spd = W(ball, 0x3A0);
                W(ball, 0x3A0) = spd - ((spd * (0x40U - ratio * 4U)) / 272U);
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                vy = SW(ball, 0xC);
                SW(ball, 0xC) = vy - ((vy * (int)(0x60 - ratio * 4)) / 288);
                break;

            case 3:
                S(ball, 0x3BC) = 0;
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                spd = W(ball, 0x3A0);
                W(ball, 0x3A0) = spd - ((spd * (0x40U - ratio * 4U)) / 288U);
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                vy = SW(ball, 0xC);
                SW(ball, 0xC) = vy - ((vy * (int)(0x48 - ratio * 4)) / 304);
                break;
            }

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            SW(ball, 0xC) = -SW(ball, 0xC);
            S(ball, 4) = -0x20;
            if (S(ball, 0x3C8) != 0) {
                S(ball, 0x3C8) += 2;
            }
            B(Scratchpad, 0x2F2) = 0;

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            if (SW(ball, 0xC) < -0x44FF) {
                S(ball, 0x3BC) = 0;
            }
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            if (SW(ball, 0xC) >= -0x5BF) {
                SW(ball, 0xC) = 0;
                B(ball, 0x396) = 2;
            }
        } else {
            /* In air: apply gravity */
            switch (S(ball, 0x3BC)) {
            case 0:
                SW(ball, 0xC) += 0x2E0;
                break;
            case 1:
                SW(ball, 0xC) += 0x2C0;
                break;
            case 2:
                SW(ball, 0xC) += 0x320;
                break;
            case 3:
                SW(ball, 0xC) += 0x338;
                break;
            }
        }

        /* Quadratic air drag: v -= (|v| * v) / (7 << 21) */
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        speed_3d = func_800B1128(SW(ball, 0x3A0) * SW(ball, 0x3A0) + SW(ball, 0xC) * SW(ball, 0xC));
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        spd = W(ball, 0x3A0);
        W(ball, 0x3A0) = spd - ((unsigned int)(speed_3d * (int)spd) / 14680064U);
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        vy = SW(ball, 0xC);
        SW(ball, 0xC) = vy - ((speed_3d * vy) / 14680064);
        break;

    case 2:
        H(ball, 0x26) = (0x80 - B(ball, 0x3A4)) << 4;
        H(ball, 0x28) -= W(ball, 0x3A0) >> 4;

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        S(ball, 2) += func_8001E694(B(ball, 0x3A4), SW(ball, 0x3A0)) >> 8;

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        S(ball, 6) += func_8001E5A4(B(ball, 0x3A4), SW(ball, 0x3A0)) >> 8;

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        SW(ball, 0xC) = 0;
        S(ball, 0x3C8) = 0;

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        if (W(ball, 0x3A0) < 0x3001U) {
            S(ball, 4) = -0x20;
        } else {
            S(ball, 4) = (signed short)(-0x20 - func_8001E6B8(8));
        }

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        H(Scratchpad, 0x2B4) = 0x7FFF;
        spd = W(ball, 0x3A0);
        spd -= spd >> 6;
        W(ball, 0x3A0) = spd;

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        spd = W(ball, 0x3A0);
        spd -= (unsigned int)((int)spd * (int)spd) / 14680064U;
        W(ball, 0x3A0) = spd;

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        if (W(ball, 0x3A0) < 0x100U) {
            B(ball, 0x396) = 0;
        }
        break;

    case 3:
    {
        unsigned char *foot_off;
        int rot_y;

        B(Scratchpad, 0x2CF + B(actor, 0x398)) = 0;
        B(Scratchpad, 0x2CF + (B(actor, 0x398) ^ 1)) = 1;
        /* 0x800FF748 + 0xAC4E = 0x8010A396 */
        B(D_800FF748, 0xAC4E) = 0xFF;

        B(ball, 0x3A4) = B(actor, 0x3AA);
        W(ball, 0x3A0) = W(actor, 0x3A0) << 8;
        SW((unsigned char *)W(Scratchpad, 0x2F4), 0xC) = 0;

        W(Scratchpad, 0x2B0) = 0;
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        H(ball, 0x26) = (0x80 - B(ball, 0x3A4)) << 4;
        H(ball, 0x28) -= W(actor, 0x3A0) << 4;

        B(Scratchpad, 0x2F2) = 0;
        H(Scratchpad, 0x2B4) = 0x7FFF;
        H(Scratchpad, 0x2B8) = 0x7FFF;
        B(Scratchpad, 0x2A9) = B(Scratchpad, 0x2A8);
        B(Scratchpad, 0x2AC) = B(Scratchpad, 0x2A8);

        foot_off = func_8004E6D4(B(actor, 0x38E), B(actor, 0x390));
        SW(Scratchpad, 0x13C) = S(actor, 2);
        SW(Scratchpad, 0x140) = S(actor, 4);
        SW(Scratchpad, 0x144) = S(actor, 6);

        H(Scratchpad, 0x124) = 0;
        rot_y = (0x1C0 - B((unsigned char *)W(Scratchpad, 0x2F4), 0x3A4)) % 256;
        H(Scratchpad, 0x126) = rot_y << 4;
        H(Scratchpad, 0x128) = 0;

        W(Scratchpad, 0x14C) = W(actor, 0x2C);
        W(Scratchpad, 0x150) = W(actor, 0x30);
        W(Scratchpad, 0x154) = W(actor, 0x34);

        func_800B1488(Scratchpad + 0x124, Scratchpad + 0x104);
        func_800B1348(Scratchpad + 0x104, Scratchpad + 0x14C);
        func_800B1318(Scratchpad + 0x104, Scratchpad + 0x13C);

        S(Scratchpad, 0x134) = (B(actor, 0x3AC) != 0) ? -S(foot_off, 0) : S(foot_off, 0);
        H(Scratchpad, 0x136) = H(foot_off, 2);
        H(Scratchpad, 0x138) = H(foot_off, 4);
        func_800B12C8(Scratchpad + 0x104, Scratchpad + 0x134, Scratchpad + 0x13C);

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        switch (B(ball, 0x3B1)) {
        case 0:
            S(ball, 2) = (S(ball, 2) * 3 + S(actor, 2) + SW(Scratchpad, 0x13C)) >> 2;
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            S(ball, 4) = (S(ball, 4) * 3 + S(actor, 4) + SW(Scratchpad, 0x140)) >> 2;
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            S(ball, 6) = (S(ball, 6) * 3 + S(actor, 6) + SW(Scratchpad, 0x144)) >> 2;
            B((unsigned char *)W(Scratchpad, 0x2F4), 0x3B1) += 1;
            break;

        case 1:
            S(ball, 2) = (S(ball, 2) * 2 + S(actor, 2) + SW(Scratchpad, 0x13C)) / 3;
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            S(ball, 4) = (S(ball, 4) * 2 + S(actor, 4) + SW(Scratchpad, 0x140)) / 3;
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            S(ball, 6) = (S(ball, 6) * 2 + S(actor, 6) + SW(Scratchpad, 0x144)) / 3;
            B((unsigned char *)W(Scratchpad, 0x2F4), 0x3B1) += 1;
            break;

        case 2:
            S(ball, 2) = (S(ball, 2) + S(actor, 2) + SW(Scratchpad, 0x13C)) >> 1;
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            S(ball, 4) = (S(ball, 4) + S(actor, 4) + SW(Scratchpad, 0x140)) >> 1;
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            S(ball, 6) = (S(ball, 6) + S(actor, 6) + SW(Scratchpad, 0x144)) >> 1;
            B((unsigned char *)W(Scratchpad, 0x2F4), 0x3B1) += 1;
            break;

        default:
            S(ball, 2) = S(actor, 2) + SW(Scratchpad, 0x13C);
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            S(ball, 4) = S(actor, 4) + SW(Scratchpad, 0x140);
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            S(ball, 6) = S(actor, 6) + SW(Scratchpad, 0x144);
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            if (B(ball, 0x3B1) < 0x20) {
                B(ball, 0x3B1) += 1;
            }
            break;
        }
        break;
    }
    }

    actor = D_800FF748 + (B(Scratchpad, 0x2AC) % 24) * 1000;
    goal_x = (B(actor, 0x399) != 0) ? -0x3320 : 0x3320;
    B((unsigned char *)W(Scratchpad, 0x2F4), 0x3A8) =
        (unsigned char)func_8005B36C(S(Scratchpad, 0x36), S(Scratchpad, 0x66), goal_x, 0);

    if (B(Scratchpad, 0x29A) == 1) {
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        if (B(ball, 0x3B2) != 0) {
            B(ball, 0x3B2) -= 1;
        } else {
            func_8004C0B0();
        }
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        if (B(ball, 0x3B0) != 0) {
            B(ball, 0x3B0) -= 1;
        }
        func_800A7450();
    } else {
        func_800A8F3C();
        if (H(Scratchpad, 0xE0) == 0) {
            int bx;
            int prev_bx;

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            bx = S(ball, 2);
            if (bx < 0) {
                bx = -bx;
            }
            if (bx >= 0x3361) {
                prev_bx = S(ball, 0x3C2);
                if (prev_bx < 0) {
                    prev_bx = -prev_bx;
                }
                if (prev_bx < 0x3361 && func_800A8B24() != 0) {
                    func_800989B4((unsigned char *)W(Scratchpad, 0x2F4));
                }
            }
        }
        phase = B(Scratchpad, 0x29A);
        if (phase != 8 && phase != 6) {
            func_800A8B94((unsigned char *)W(Scratchpad, 0x2F4));
        }
    }

    func_8004B414();
}
