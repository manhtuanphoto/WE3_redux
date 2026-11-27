/*
 * func_8009E664 - AI off-ball support / forward run positioning update.
 *
 * Purpose:
 *   Updates an AI player's target coordinates (scratch 0xF8/0xFC) and calls
 *   func_80068FC0 to commit the off-ball positioning target. Uses a staggered
 *   modulo schedule based on frame counter and player ID: on scheduled ticks,
 *   computes a direct forward-support target and may set offside/trap flag bits
 *   in +0x3D4; on other ticks, clamps X against the team's offside/defensive line.
 *
 * Arguments:
 *   actor (unsigned char *): player actor struct (1000 bytes).
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F800022[2] (u16, R): team mask halfword indexed by side (ORed into +0x3D4).
 *   - 0x1F8000F8 (s16, R/W): target X coordinate.
 *   - 0x1F8000FC (s16, R/W): target Z coordinate.
 *   - 0x1F800290 (u32, R): match frame counter.
 *   - 0x1F8002F4 (u8 *, R): pointer to ball actor.
 *   - 0x1F8002FF[2] (u8, R): team line offset byte indexed by side.
 *   - 0x1F800314 (s32, R): defensive/offside X line for attack dir 1.
 *   - 0x1F800318 (s32, R): defensive/offside X line for attack dir 0.
 *
 * Actor fields reads/writes:
 *   - actor: +0x02/+0x06 (s16 x/z, R), +0x38D (u8 id, R), +0x392 (u8 team slot, R),
 *            +0x398 (u8 side, R), +0x399 (u8 attack direction flag, R),
 *            +0x39C (u8 target active/mode flag, W), +0x3AA (u8 body heading, R),
 *            +0x3D4 (u16 flags, R/W), +0x3E2/+0x3E3 (u8 flags, W=0).
 *   - ball:  +0x02/+0x06 (s16 x/z, R).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x20E2 (8418 raw = 33.672 m) = ball X threshold beyond which target X clamps to 0x28E2.
 *   - 0xA00 (2560 raw = 10.24 m) = forward lead offset from ball X.
 *   - 0x28E2 (10466 raw = 41.864 m) = deep-attacking X target.
 *   - 300 frames (10.0 s) = periodic line-clamp relaxation window.
 *   - 0x280 (640 raw = 2.56 m) + (scratch[0x2FF+side] << 7) = offside/line margin.
 *   - 0x1991 (6545 raw = 26.18 m), 0x23E2 (9186 raw = 36.744 m) = deep-zone X checks.
 *   - 0x10 (16 angle units = 22.5 deg) = alignment tolerance with attack direction.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_8010C1D8[];
extern unsigned char D_8010C1DC[];
extern unsigned char D_8010C328[];

extern unsigned int func_8009D444(unsigned char *actor);
extern void func_80068FC0(unsigned char *actor, int x, int z);

void func_8009E664(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *ball;
    int dir_sign;
    int offset;
    int stat_low;
    unsigned int rem;
    unsigned int mod_id;

    if ((func_8009D444(actor) & 0xFF) != 0) {
        return;
    }

    dir_sign = (B(actor, 0x399) != 0) ? -1 : 1;

    offset = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
    stat_low = (int)(W(D_8010C328 + offset) & 0xF);
    rem = W(scratch, 0x290) % (unsigned int)(stat_low * 11);
    mod_id = (unsigned int)(B(actor, 0x38D) % 22U) & 0xFF;

    B(actor, 0x39C) = 0;

    if (mod_id != rem) {
        int ball_x, abs_bx;
        int z_target;
        int frame_cnt;

        ball = (unsigned char *)W(scratch, 0x2F4);
        ball_x = S(ball, 2);
        abs_bx = (ball_x < 0) ? -ball_x : ball_x;

        if (abs_bx < 0x20E2) {
            S(scratch, 0xF8) = (signed short)(ball_x + dir_sign * 0xA00);
            if (B(actor, 0x38D) == B(D_8010C1DC, 0)) {
                z_target = (S(ball, 6) + S(actor, 6) * 2) / 3;
            } else {
                z_target = (S(ball, 6) + S(actor, 6) * 5) / 6;
            }
        } else {
            S(scratch, 0xF8) = (signed short)(dir_sign * 0x28E2);
            z_target = (S(actor, 6) * 2) / 3;
        }
        S(scratch, 0xFC) = (signed short)z_target;

        frame_cnt = SW(scratch, 0x290);
        offset = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        stat_low = (int)(W(D_8010C328 + offset) & 0xF);

        if ((frame_cnt % 300) > (stat_low * 2 + 0x1E)) {
            if (B(actor, 0x399) == 0 && SW(scratch, 0x318) < S(scratch, 0xF8)) {
                S(scratch, 0xF8) = (signed short)SW(scratch, 0x318);
            } else if (B(actor, 0x399) == 1 && S(scratch, 0xF8) < SW(scratch, 0x314)) {
                S(scratch, 0xF8) = (signed short)SW(scratch, 0x314);
            }
        } else {
            if (B(actor, 0x399) == 0) {
                int limit = SW(scratch, 0x318) + (B(scratch, 0x2FF + B(actor, 0x398)) << 7) + 0x280;
                if (limit < S(scratch, 0xF8)) {
                    S(scratch, 0xF8) = (signed short)limit;
                }
            } else if (B(actor, 0x399) == 1) {
                int limit = SW(scratch, 0x314) - ((B(scratch, 0x2FF + B(actor, 0x398)) << 7) + 0x280);
                if (S(scratch, 0xF8) < limit) {
                    S(scratch, 0xF8) = (signed short)limit;
                }
            }
        }

        if (B(actor, 0x38D) != B(D_8010C1D8, 4)) {
            S(scratch, 0xFC) = S(actor, 6);
        }
        func_80068FC0(actor, S(scratch, 0xF8), S(scratch, 0xFC));
        return;
    }

    {
        int ball_x, abs_bx;
        int goal_dir, heading, diff;
        int facing_goal = 0;

        ball = (unsigned char *)W(scratch, 0x2F4);
        ball_x = S(ball, 2);
        abs_bx = (ball_x < 0) ? -ball_x : ball_x;

        if (abs_bx < 0x20E2) {
            S(scratch, 0xF8) = (signed short)(ball_x + dir_sign * 0xA00);
            S(scratch, 0xFC) = (signed short)((S(ball, 6) + S(actor, 6) * 2) / 3);
        } else {
            S(scratch, 0xF8) = (signed short)(dir_sign * 0x28E2);
            S(scratch, 0xFC) = 0;
        }
        func_80068FC0(actor, S(scratch, 0xF8), S(scratch, 0xFC));

        goal_dir = B(actor, 0x399) << 7;
        heading = B(actor, 0x3AA);
        diff = (heading - goal_dir) & 0xFF;
        if ((unsigned int)diff < 0x81U) {
            if (diff < 0x10) {
                facing_goal = 1;
            }
        } else {
            if (((goal_dir - heading) & 0xFF) < 0x10) {
                facing_goal = 1;
            }
        }

        if (facing_goal) {
            int bx = S(W(scratch, 0x2F4), 2);
            int ax = S(actor, 2);
            if (bx < 0) {
                bx = -bx;
            }
            if (ax < 0) {
                ax = -ax;
            }
            if (bx >= 0x1991 && ax < 0x23E2) {
                H(actor, 0x3D4) |= H(scratch, 0x22 + (B(actor, 0x398) << 1));
            }
        }

        B(actor, 0x39C) = 1;
        if (B(actor, 0x38D) != B(D_8010C1D8, 4)) {
            B(actor, 0x3E2) = 0;
            B(actor, 0x3E3) = 0;
        }
    }
}
