/*
 * func_800A1E50
 * Purpose: Updates team tactical/mentality setting via func_800A2310(side, level) based on
 *          team possession, ball/actor X position, score differential, difficulty/mode (0x2DF),
 *          and elapsed/remaining match time (func_8004F3CC() / 3600).
 * Arguments:
 *   actor - pointer to 1000-byte player actor struct
 * Return value:
 *   void
 * Scratchpad reads/writes:
 *   R: 0x0A, 0x29A (match phase), 0x2CF[side] (possession), 0x2D1[side] (score),
 *      0x2DF (tactical/difficulty setting), 0x2F4 (ball actor ptr), 0x2FF[side]
 *   W: none directly (delegates to func_800A2310)
 * Actor reads/writes:
 *   R: +0x02 (s16 x), +0x398 (u8 side), +0x399 (u8 attack direction)
 *   W: none
 * Constants:
 *   0x1990 = 6544 raw (26.176 m);
 *   0x2E21 = 11809 raw (47.236 m); 0x2B21 = 11041 raw (44.164 m);
 *   0x2921 = 10529 raw (42.116 m); 0x2721 = 10017 raw (40.068 m);
 *   3600 (frames = 120 s at 30 Hz, or time units), threshold 0x24 (36).
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCH  ((unsigned char *)0x1F800000)

extern int func_8001E6B8(int n);
extern unsigned int func_8004F3CC(void);
extern void func_800A2310(unsigned int side, unsigned int mode);

void func_800A1E50(unsigned char *actor) {
    unsigned int side;
    unsigned int side_b;
    unsigned int time_val;
    unsigned char *ball;
    int atk_dir;
    int actor_x;
    int ball_x;
    int abs_bx;
    unsigned int own_score;
    unsigned int opp_score;
    int diff;
    int abs_diff;

    side = B(actor, 0x398);
    time_val = func_8004F3CC();

    if (B(SCRATCH, 0x2CF + side) == 0) {
        if (func_8001E6B8(5) == 0) {
            atk_dir = B(actor, 0x399);
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            ball_x = S(ball, 2);
            if ((atk_dir == 0 && ball_x < 0) || (atk_dir == 1 && ball_x > 0)) {
                func_800A2310(side, 1);
                return;
            }
        }

        atk_dir = B(actor, 0x399);
        actor_x = S(actor, 2);
        if (atk_dir == 0) {
            if (actor_x < -0x1990) { /* -26.176 m */
                return;
            }
        } else {
            if (-actor_x < -0x1990) {
                return;
            }
        }

        side_b = side & 0xFF;
        own_score = B(SCRATCH, 0x2D1 + side_b);
        opp_score = B(SCRATCH, 0x2D1 + (side ^ 1));
        diff = (int)(own_score - opp_score);

        if (own_score < opp_score) {
            if (B(SCRATCH, 0x29A) == 4) {
                func_800A2310(side_b, 2);
                return;
            }
            if (B(SCRATCH, 0x2DF) == 0) {
                func_800A2310(side_b, 1);
                return;
            }
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            abs_bx = S(ball, 2);
            if (abs_bx < 0) {
                abs_bx = -abs_bx;
            }
            if (abs_bx >= 0x2E21) { /* 47.236 m */
                func_800A2310(side_b, 2);
            }
            return;
        }

        abs_diff = (diff >= 0) ? diff : -diff;
        if (abs_diff >= 2) {
            func_800A2310(side_b, 1);
            return;
        }

        if (abs_diff > 0) {
            switch (B(SCRATCH, 0x2DF)) {
            case 0:
                func_800A2310(side_b, func_8001E6B8(2) & 0xFF);
                return;
            case 1:
                if ((time_val / 3600) >= 0x24) {
                    func_800A2310(side_b, 0);
                }
                return;
            default:
                return;
            }
        }

        /* Tied score (abs_diff == 0) */
        switch (B(SCRATCH, 0x2DF)) {
        case 0:
            func_800A2310(side_b, 1);
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            abs_bx = S(ball, 2);
            if (abs_bx < 0) {
                abs_bx = -abs_bx;
            }
            if (abs_bx >= 0x2E21) { /* 47.236 m */
                func_800A2310(side_b, 2);
            }
            return;
        case 1:
            func_800A2310(side_b, (func_8001E6B8(2) + 1) & 0xFF);
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            abs_bx = S(ball, 2);
            if (abs_bx < 0) {
                abs_bx = -abs_bx;
            }
            if (abs_bx >= 0x2B21) { /* 44.164 m */
                func_800A2310(side_b, 2);
            }
            return;
        case 2:
            func_800A2310(side_b, (func_8001E6B8(2) + 1) & 0xFF);
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            abs_bx = S(ball, 2);
            if (abs_bx < 0) {
                abs_bx = -abs_bx;
            }
            if (abs_bx >= 0x2921) { /* 42.116 m */
                func_800A2310(side_b, 2);
            }
            return;
        case 3:
            func_800A2310(side_b, (func_8001E6B8(2) + 1) & 0xFF);
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            abs_bx = S(ball, 2);
            if (abs_bx < 0) {
                abs_bx = -abs_bx;
            }
            if (abs_bx >= 0x2721) { /* 40.068 m */
                func_800A2310(side_b, 2);
            }
            return;
        default:
            return;
        }
    } else {
        side_b = side & 0xFF;
        if (B(SCRATCH, 0x0A) != 0 && func_8001E6B8(5) == 0) {
            if (B(SCRATCH, 0x2FF + side_b) == 2) {
                func_800A2310(side_b, 1);
            }
        }

        own_score = B(SCRATCH, 0x2D1 + side_b);
        opp_score = B(SCRATCH, 0x2D1 + (side ^ 1));
        diff = (int)(own_score - opp_score);

        if (own_score < opp_score) {
            if (B(SCRATCH, 0x2DF) == 0) {
                func_800A2310(side_b, 1);
            } else {
                func_800A2310(side_b, (func_8001E6B8(2) + 1) & 0xFF);
            }
            return;
        }

        abs_diff = (diff >= 0) ? diff : -diff;
        if (abs_diff >= 2) {
            func_800A2310(side_b, 1);
            return;
        }

        if (abs_diff > 0) {
            if (B(SCRATCH, 0x2DF) == 0) {
                func_800A2310(side_b, 1);
            } else {
                func_800A2310(side_b, func_8001E6B8(2) & 0xFF);
            }
            return;
        }

        switch (B(SCRATCH, 0x2DF)) {
        case 0:
            func_800A2310(side_b, 1);
            return;
        case 1:
            if ((time_val / 3600) >= 0x24) {
                func_800A2310(side_b, 2);
            } else {
                func_800A2310(side_b, func_8001E6B8(2) & 0xFF);
            }
            return;
        default:
            func_800A2310(side, func_8001E6B8(2) & 0xFF);
            return;
        }
    }
}
