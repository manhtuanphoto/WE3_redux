/*
 * func_80069B28
 * Purpose: Computes off-ball attacking run target coordinates (SCRATCH+0xF8, 0xFC) for an
 *          AI player based on match sub-phase/tactical zone (SCRATCH+0x30B), team tactic
 *          (SCRATCH+0x2FD[side]), player role (actor+0x391), and ball position.
 * Arguments:
 *   actor - pointer to 1000-byte player actor struct
 * Return value:
 *   void
 * Scratchpad reads/writes:
 *   R: 0x2F4 (ball actor ptr), 0x2FD[side] (team tactic), 0x2FF[side] (mentality),
 *      0x30B (attacking zone/mode 0..5), 0x30C[dir] (s16 line x), 0x310[dir] (s16 line x)
 *   W: 0xF8 (s16 target x), 0xFC (s16 target z)
 * Actor reads/writes:
 *   R: +0x391 (u8 tactical role), +0x398 (u8 side), +0x399 (u8 attack direction),
 *      +0x3DA (u8 tick/counter)
 *   W: +0x3DC (s16 jitter x), +0x3DE (s16 jitter z)
 * Constants:
 *   0x28F5 (10485 raw = 41.94 m) X clamp;
 *   0x500 (1280 raw = 5.12 m), 0x700 (1792 raw = 7.168 m) forward X offsets;
 *   0x100 (256 raw = 1.024 m), 0x280 (640 raw = 2.56 m), 0x400 (1024 raw = 4.096 m) jitter steps.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCH  ((unsigned char *)0x1F800000)

extern int func_8001E6B8(int n);
extern int func_8005A76C(int cur, int target);
extern void func_800A3450(unsigned char *actor);

void func_80069B28(unsigned char *actor) {
    int dir_sign;
    unsigned char *ball;
    int ball_z;
    int fz;
    int fx;
    int role;
    int tactic;
    int step_x;
    int unused_mode_flag;

    dir_sign = (B(actor, 0x399) == 0) ? 1 : -1;
    func_800A3450(actor);

    switch (B(SCRATCH, 0x30B)) {
    case 0:
        S(SCRATCH, 0xF8) = (signed short)((S(SCRATCH, 0xF8) + S(SCRATCH, 0x310 + (B(actor, 0x399) << 1))) / 2);
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        S(SCRATCH, 0xFC) = (signed short)func_8005A76C(S(SCRATCH, 0xFC), S(ball, 6));
        break;

    case 1:
    case 2:
    case 3:
        S(actor, 0x3DE) = 0;
        S(actor, 0x3DC) = 0;
        break;

    case 4:
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        ball_z = S(ball, 6);
        role = B(actor, 0x391);
        if (ball_z * (signed short)dir_sign < 0) {
            if (role == 9 || role == 0xC) {
                S(SCRATCH, 0xFC) = (signed short)(S(SCRATCH, 0xFC) / 3);
            }
        } else {
            if ((unsigned int)(role - 0xA) < 2) {
                S(SCRATCH, 0xFC) = (signed short)(S(SCRATCH, 0xFC) / 3);
            }
        }

        if ((B(actor, 0x3DA) % 3) == 0) {
            switch (B(SCRATCH, 0x2FF + B(actor, 0x398))) {
            case 0:
                step_x = (dir_sign << 16) >> 8; /* +-256 raw (+-1.024 m) */
                S(actor, 0x3DC) = (signed short)(func_8001E6B8(3) * step_x);
                break;
            case 1:
                step_x = ((signed short)dir_sign * 5) << 7; /* +-640 raw (+-2.56 m) */
                S(actor, 0x3DC) = (signed short)(func_8001E6B8(3) * step_x);
                break;
            case 2:
                step_x = (dir_sign << 16) >> 6; /* +-1024 raw (+-4.096 m) */
                S(actor, 0x3DC) = (signed short)(func_8001E6B8(3) * step_x);
                break;
            }
            S(actor, 0x3DE) = (signed short)((func_8001E6B8(9) << 8) - 0x400);
        }

        S(SCRATCH, 0xF8) = S(SCRATCH, 0xF8) + S(actor, 0x3DC);
        S(SCRATCH, 0xFC) = S(SCRATCH, 0xFC) + S(actor, 0x3DE);
        fx = S(SCRATCH, 0xF8);
        if ((fx >= 0 ? fx : -fx) >= 0x28F6) { /* >= 41.944 m */
            S(SCRATCH, 0xF8) = (fx >= 0) ? 0x28F5 : -0x28F5; /* +-41.94 m */
        }
        break;

    case 5:
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        S(SCRATCH, 0xF8) = (signed short)func_8005A76C(S(SCRATCH, 0xF8), S(ball, 2));
        S(SCRATCH, 0xFC) = (signed short)(S(SCRATCH, 0xFC) / 2);
        break;
    }

    /* Dead/unused value computed by jtbl_800127A0 in original binary */
    switch (B(SCRATCH, 0x30B)) {
    case 0:
    case 1:
        break;
    case 2:
        unused_mode_flag = 1;
        break;
    case 3:
        unused_mode_flag = 2;
        break;
    case 4:
    case 5:
        unused_mode_flag = 4;
        break;
    default:
        unused_mode_flag = 0;
        break;
    }
    (void)unused_mode_flag;

    tactic = B(SCRATCH, 0x2FD + B(actor, 0x398));
    if (tactic == 4 || tactic == 0) {
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        fz = S(SCRATCH, 0xFC);
        if (((S(ball, 6) >= 0) ? 1 : -1) != ((fz >= 0) ? 1 : -1)) {
            S(SCRATCH, 0xFC) = (signed short)((fz * 4) / 5);
        }

        switch (B(SCRATCH, 0x30B)) {
        case 0:
        case 1:
            S(SCRATCH, 0xF8) = (signed short)((S(SCRATCH, 0x310 + (B(actor, 0x399) << 1)) +
                                               S(SCRATCH, 0x30C + (B(actor, 0x399) << 1))) / 2);
            break;
        case 2:
        case 3:
            S(SCRATCH, 0xF8) = (signed short)(S(SCRATCH, 0xF8) + ((signed short)dir_sign * 5) * 0x100); /* +-5.12 m */
            break;
        case 4:
        case 5:
            S(SCRATCH, 0xF8) = (signed short)(S(SCRATCH, 0xF8) + ((signed short)dir_sign * 7) * 0x100); /* +-7.168 m */
            break;
        }
    } else if (tactic >= 0xB && tactic < 0x10) {
        S(SCRATCH, 0xF8) = (signed short)func_8005A76C(
            S(SCRATCH, 0xF8),
            S(SCRATCH, 0x30C + (B(actor, 0x399) << 1)));
    }
}
