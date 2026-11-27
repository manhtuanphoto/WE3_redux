/*
 * func_8006B46C
 *
 * Purpose:
 *   Computes and clamps the AI target X/Z coordinates in Scratchpad+0xF8 and
 *   Scratchpad+0xFC for an actor based on team direction, ball position,
 *   actor state (actor+0x391), and team line bounds (Scratchpad+0x30C/0x30E).
 *
 * Arguments:
 *   actor: (unsigned char*) pointer to player entity struct
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x009 (u8)  flag disabling +0x400 team-line clamp when nonzero (R)
 *     +0x0F8 (s16) AI target X coordinate (R/W)
 *     +0x0FC (s16) AI target Z coordinate (R/W)
 *     +0x2A9 (u8)  flag disabling ball-X clamp when nonzero (R)
 *     +0x2F4 (ptr) pointer to ball entity (+0x02 s16 x, +0x06 s16 z) (R)
 *     +0x30C (s16[2]) team boundary X coordinates (R)
 *   actor:
 *     +0x38D (u8)  player ID (compared against D_8010C1E4)
 *     +0x391 (u8)  player state/action (jump-table switch for 0x11..0x19)
 *     +0x399 (u8)  team/side (0 or 1; determines attack direction sign)
 *
 * Magic numbers:
 *   0x0BC0 (3008): ball X threshold to skip target update
 *   0x0B00 (2816), 0x0600 (1536): X offsets from ball / target
 *   0x23E2 (9186): max absolute X clamp
 *   0x0300 (768), 0x0400 (1024), 0x0200 (512): state/line clamp offsets
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define P(p,o)  (*(unsigned char**)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_8010C1E4;
extern signed short func_8005A76C(int cur_z, int ball_z, int mode);

void func_8006B46C(unsigned char *actor) {
    signed short dir = -1;
    unsigned char side = B(actor, 0x399);
    unsigned char *ball;
    signed short ball_x;
    int skip_update = 0;

    if (side == 0) {
        dir = 1;
        ball = P(Scratchpad, 0x2F4);
        ball_x = S(ball, 0x02);
        if (ball_x >= 0x0BC1) {
            skip_update = 1;
        }
    } else if (side == 1) {
        ball = P(Scratchpad, 0x2F4);
        ball_x = S(ball, 0x02);
        if (ball_x < -0x0BC0) {
            skip_update = 1;
        }
    }

    if (!skip_update) {
        if (B(actor, 0x38D) == D_8010C1E4) {
            ball = P(Scratchpad, 0x2F4);
            S(Scratchpad, 0x0F8) = (signed short)(S(ball, 0x02) - (dir * 0x0B00));
            S(Scratchpad, 0x0FC) = func_8005A76C(S(Scratchpad, 0x0FC), S(ball, 0x06), 3);
        } else {
            S(Scratchpad, 0x0F8) = (signed short)(S(Scratchpad, 0x0F8) - (dir * 0x0600));
        }

        side = B(actor, 0x399);
        ball = P(Scratchpad, 0x2F4);
        ball_x = S(ball, 0x02);
        if ((side == 0 && ball_x < 0) || (side == 1 && ball_x > 0)) {
            int target_dir = dir * S(Scratchpad, 0x0F8);
            signed short line_x = S(Scratchpad, 0x30C + ((side ^ 1) << 1));
            int line_dir = dir * line_x;

            if (line_dir < target_dir + 0x200) {
                S(Scratchpad, 0x0F8) = (signed short)(line_x + (dir << 9));
            }

            {
                signed short tx = S(Scratchpad, 0x0F8);
                int abs_tx = (tx >= 0) ? tx : -tx;
                if (abs_tx >= 0x23E3) {
                    S(Scratchpad, 0x0F8) = (tx >= 0) ? 0x23E2 : -0x23E2;
                }
            }
        }
    }

    {
        unsigned int state_off = (unsigned int)(B(actor, 0x391) - 0x11);
        switch (state_off) {
        case 0:
        case 1:
        case 4:
        case 5:
        case 6:
        case 7:
            if (S(Scratchpad, 0x0F8) * dir > 0) {
                S(Scratchpad, 0x0F8) = 0;
            }
            break;
        case 8:
            if (S(Scratchpad, 0x0F8) * dir >= 0x301) {
                S(Scratchpad, 0x0F8) = (signed short)(dir * 0x300);
            }
            break;
        case 2:
        case 3:
        default:
            break;
        }
    }

    if (B(Scratchpad, 0x009) == 0) {
        if (B(actor, 0x399) == 0) {
            signed short bound = S(Scratchpad, 0x30E);
            if (S(Scratchpad, 0x0F8) > bound + 0x400) {
                S(Scratchpad, 0x0F8) = (signed short)(bound + 0x400);
            }
        } else {
            signed short bound = S(Scratchpad, 0x30C);
            if (S(Scratchpad, 0x0F8) < bound - 0x400) {
                S(Scratchpad, 0x0F8) = (signed short)(bound - 0x400);
            }
        }
    }

    if (B(Scratchpad, 0x2A9) == 0) {
        ball = P(Scratchpad, 0x2F4);
        ball_x = S(ball, 0x02);
        if (B(actor, 0x399) == 0) {
            if (ball_x < S(Scratchpad, 0x0F8)) {
                S(Scratchpad, 0x0F8) = ball_x;
            }
        } else {
            if (S(Scratchpad, 0x0F8) < ball_x) {
                S(Scratchpad, 0x0F8) = ball_x;
            }
        }
    }
}
