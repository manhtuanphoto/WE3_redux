/*
 * func_800A0F30 - AI man-marking / pressing target coordinate calculation.
 *
 * Purpose:
 *   Computes an AI defender's target (X, Z) coordinates when marking or pressing
 *   the assigned opponent (+0x3E1 % 24) based on the opponent's state, team
 *   tactical/pressing mode (scratch 0x30B), player stat level, ball position,
 *   and match phase (open play = 1 vs phase 4), clamping X inside the pitch.
 *
 * Arguments:
 *   actor (unsigned char *): AI defender actor struct (1000 bytes).
 *   unused_a1 (int): unused ($a1 overwritten before use).
 *   out_x (signed short *): output target X coordinate.
 *   out_z (signed short *): output target Z coordinate.
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F8000FE/100 (s16, R): base marking target X/Z when in tight/trap mode.
 *   - 0x1F80029A (u8, R): match phase (1 = open play, 4 = set-piece/restart).
 *   - 0x1F8002A9 (u8, R): current ball owner ID.
 *   - 0x1F8002F4 (u8 *, R): pointer to ball actor.
 *   - 0x1F80030B (u8, R): tactical/pressing mode (0..5).
 *   - 0x1F800314/318 (s32, R): defensive/offside X line indexed by attack dir.
 *   - 0x1F800328 (u8, R): match/team mode flag (2 skips saved-pos preference).
 *
 * Actor fields reads/writes:
 *   - actor:  +0x02 (s16 x, R), +0x392 (u8 team slot, R), +0x398 (u8 side, R),
 *             +0x399 (u8 attack direction flag, R), +0x3E1 (u8 mark target idx, R/W),
 *             +0x3E2/+0x3E3 (u8 flags, W=0 when target owns ball).
 *   - target: +0x02/+0x06 (s16 x/z, R), +0x38D (u8 id, R), +0x396 (u8 state, R),
 *             +0x3BC/+0x3BE (s16 saved x/z, R).
 *   - ball:   +0x02/+0x06 (s16 x/z, R).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x100..0x500 (256..1280 raw = 1.024..5.12 m) = base goal-side cushion.
 *   - 40 (0.16 m) * top_nibble(D_8010C328) = stat-based cushion adjustment.
 *   - 0x8B6 (2230 raw = 8.92 m) = ball Z threshold for Z-tracking toward ball.
 *   - 0x500 (1280 raw = 5.12 m) = maximum goal-side lag behind ball X.
 *   - 0x80 (128 raw = 0.512 m) = Z offset in match phase 4.
 *   - 0x2E08 (11784 raw = 47.136 m) = X-axis pitch clamp.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];
extern unsigned char D_8010C328[];

extern int func_8001E6B8(int n);
extern int func_8005A76C(int cur, int target, int step);
extern int func_8005A7E4(int cur, int target, int shift);

void func_800A0F30(unsigned char *actor, int unused_a1, signed short *out_x, signed short *out_z)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *target;
    unsigned char *ball;
    int dir_sign;
    int flag = 0;
    unsigned int cushion = 0;
    unsigned int t_state;
    unsigned int mode;
    int offset;
    int stat_top;
    unsigned int phase;

    (void)unused_a1;
    dir_sign = (B(actor, 0x399) == 0) ? 1 : -1;
    target = D_800FF748 + (B(actor, 0x3E1) % 24U) * 1000;

    t_state = B(target, 0x396);
    mode = B(scratch, 0x30B);

    if (t_state == 3) {
        switch (mode) {
        case 0:
        case 1: cushion = 0x500; break;
        case 2: cushion = 0x400; break;
        case 3: cushion = 0x380; break;
        case 4:
        case 5: cushion = 0x280; break;
        default: break;
        }
    } else if (t_state == 1 || t_state == 0xB || t_state == 0x1D) {
        if (t_state == 0xB || t_state == 0x1D) {
            flag = 1;
        }
        switch (mode) {
        case 0: cushion = 0x480; break;
        case 1: cushion = 0x380; break;
        case 2: cushion = 0x280; break;
        case 3: cushion = 0x220; break;
        case 4:
        case 5: cushion = 0x120; break;
        default: break;
        }
    } else {
        switch (mode) {
        case 0:
        case 1: cushion = 0x380; break;
        case 2: cushion = 0x280; break;
        case 3: cushion = 0x200; break;
        case 4:
        case 5: cushion = 0x100; break;
        default: break;
        }
    }

    offset = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
    stat_top = (int)(W(D_8010C328 + offset) >> 28);
    cushion += (unsigned int)(stat_top * 40);

    if ((flag & 0xFF) != 0) {
        *out_x = (signed short)(S(scratch, 0xFE) - (int)(cushion & 0xFFFF) * dir_sign);
        *out_z = S(scratch, 0x100);
    } else {
        switch (B(scratch, 0x30B)) {
        case 0:
        case 1:
            flag = 1;
            break;
        case 2:
        case 3:
        case 5:
            flag = 0;
            break;
        case 4:
            break;
        }

        offset = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        stat_top = (int)(W(D_8010C328 + offset) >> 28);
        if (func_8001E6B8(10 - stat_top) == 0 || (flag & 0xFF) != 0 || B(scratch, 0x328) == 2) {
            *out_x = (signed short)(S(target, 0x3BC) - (int)(cushion & 0xFFFF) * dir_sign);
        } else {
            *out_x = (signed short)(S(target, 2) - (int)(cushion & 0xFFFF) * dir_sign);
        }
        *out_z = S(target, 0x3BE);
    }

    phase = B(scratch, 0x29A);
    if (phase == 1) {
        *out_x = (signed short)func_8005A76C(*out_x, SW(scratch, 0x314 + (B(actor, 0x399) << 2)), 5);
        if (B(target, 0x38D) == B(scratch, 0x2A9)) {
            B(actor, 0x3E2) = 0;
            B(actor, 0x3E3) = 0;
            B(actor, 0x3E1) = 0;
        }

        switch (B(scratch, 0x30B)) {
        case 0:
            B(actor, 0x3E1) = 0;
            break;
        case 2:
        {
            int bz, abs_bz;
            ball = (unsigned char *)W(scratch, 0x2F4);
            bz = S(ball, 6);
            abs_bz = (bz < 0) ? -bz : bz;
            if (abs_bz < 0x8B6) {
                *out_z = (signed short)func_8005A76C(*out_z, bz, 1);
            }
            break;
        }
        case 3:
        {
            int bz, abs_bz;
            ball = (unsigned char *)W(scratch, 0x2F4);
            bz = S(ball, 6);
            abs_bz = (bz < 0) ? -bz : bz;
            if (abs_bz < 0x8B6) {
                *out_z = (signed short)func_8005A76C(*out_z, bz, 2);
            }
            break;
        }
        case 5:
        {
            int dx = S(actor, 2) - S(target, 2);
            if (dx < 0) {
                dx = -dx;
            }
            ball = (unsigned char *)W(scratch, 0x2F4);
            *out_z = (signed short)func_8005A7E4(*out_z, S(ball, 6), dx >> 7);
            break;
        }
        case 1:
        case 4:
        default:
            break;
        }

        ball = (unsigned char *)W(scratch, 0x2F4);
        if (B(actor, 0x399) != 0) {
            int bx = S(ball, 2);
            if (*out_x < bx + 0x500) {
                *out_x = (signed short)(bx + 0x500);
            }
        } else {
            int bx = S(ball, 2);
            if (bx - 0x500 < *out_x) {
                *out_x = (signed short)(bx - 0x500);
            }
        }
    } else if (phase == 4) {
        *out_x = S(target, 0x3BC);
        ball = (unsigned char *)W(scratch, 0x2F4);
        if (S(ball, 6) - S(target, 6) >= 0) {
            *out_z = (signed short)(S(target, 0x3BE) + 0x80);
        } else {
            *out_z = (signed short)(S(target, 0x3BE) - 0x80);
        }
    }

    {
        int x_val = *out_x;
        int abs_x = (x_val < 0) ? -x_val : x_val;
        if (abs_x >= 0x2E09) {
            *out_x = (x_val >= 0) ? 0x2E08 : -0x2E08;
        }
    }
}
