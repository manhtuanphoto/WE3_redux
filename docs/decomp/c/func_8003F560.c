/*
 * func_8003F560
 *
 * Purpose:
 *   Evaluates whether an AI defender should attempt a slide tackle (actor state
 *   0x0D). Checks eligibility, ball height, every-5th-frame gate, difficulty-
 *   scaled RNG probability, squared distance to ball, facing angle, and
 *   relative ball direction.
 *
 * Arguments:
 *   actor (u8*): defending player actor pointer
 *
 * Return value:
 *   1 if slide tackle state (0x0D) is triggered; 0 otherwise.
 *
 * Actor fields read:
 *   +0x002 (s16) X position
 *   +0x006 (s16) Z position
 *   +0x399 (u8)  team side (0 or 1)
 *   +0x39A (u8)  active/eligible flag
 *   +0x3AA (u8)  facing angle (0..255)
 *
 * Actor fields written:
 *   +0x396 (u8)  state = 0x0D on trigger
 *   +0x397 (u8)  substate = 0 on trigger
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x290 (s32) frame counter (checked % 5 == 0)
 *   +0x2A9 (u8)  ball possession flag/holder
 *   +0x2F4 (u32) ball pointer (+0x02 X, +0x04 Y, +0x06 Z, +0x3A0 speed, +0x3A4 dir)
 *   +0x328 (u8)  AI difficulty (0, 1, >=2)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned int func_8001E730(unsigned int threshold);
extern unsigned int func_8005B36C(signed short x1, signed short z1, signed short x2, signed short z2);

unsigned int func_8003F560(unsigned char *actor)
{
    unsigned char *ball;
    signed short adj_bx;
    signed int dx, dz;
    unsigned int dist2;
    unsigned int ang_to_ball;
    unsigned int diff;
    unsigned int ball_diff;
    unsigned char diff_level;
    unsigned int prob;

    if (B(actor, 0x39A) == 0) {
        return 0;
    }
    if (B(Scratchpad, 0x2A9) == 0) {
        return 0;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    if (S(ball, 4) < -0xA0) {
        return 0;
    }

    adj_bx = S(ball, 2);
    if (B(actor, 0x399) != 0) {
        adj_bx = (signed short)-adj_bx;
    }

    if ((SW(Scratchpad, 0x290) % 5) != 0) {
        return 0;
    }

    diff_level = B(Scratchpad, 0x328);
    if (diff_level == 1) {
        prob = (adj_bx < -0x1000) ? 0xAU : 0x14U;
    } else if (diff_level < 2) {
        return 0;
    } else {
        prob = (adj_bx < -0x1000) ? 0x14U : 0x28U;
    }

    if ((func_8001E730(prob) & 0xFFU) == 0) {
        return 0;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    dx = S(actor, 2) - S(ball, 2);
    dz = S(actor, 6) - S(ball, 6);
    dist2 = (unsigned int)(dx * dx + dz * dz);
    if ((signed int)dist2 > 0x64000) {
        return 0;
    }

    ang_to_ball = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));
    diff = (ang_to_ball - B(actor, 0x3AA)) & 0xFFU;
    if (diff >= 0x81U) {
        diff = (B(actor, 0x3AA) - ang_to_ball) & 0xFFU;
    }
    if (diff >= 0x21U) {
        return 0;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    {
        unsigned char ball_dir = B(ball, 0x3A4);
        unsigned int raw = (ang_to_ball - ball_dir) & 0xFFU;
        ball_diff = (raw < 0x81U) ? raw : ((ball_dir - ang_to_ball) & 0xFFU);
    }

    if (B(Scratchpad, 0x2A9) != 0) {
        if ((ball_diff & 0xFFU) < 0x41U) {
            return 0;
        }
        B(actor, 0x396) = 0xD;
        B(actor, 0x397) = 0;
        return 1;
    }

    /* Dead branch relative to line 11 check, preserved verbatim */
    if ((ball_diff & 0xFFU) < 0x60U) {
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        if (W(ball, 0x3A0) > 0xA000U) {
            B(actor, 0x396) = 0xD;
            B(actor, 0x397) = 0;
            return 1;
        }
    }

    return 0;
}
