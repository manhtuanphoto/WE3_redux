/*
 * func_800552F4
 *
 * Purpose:
 *   Checks whether a player actor should attempt a sliding/diving interception
 *   or block on a fast-moving free ball (SCRATCH+0x2A9 == 0, flight timer >= 17,
 *   ball speed >= 0x5800). Verifies distance/height/angle constraints and whether
 *   the actor is at least as close to (SCRATCH+0xF8, SCRATCH+0xFC) as the nearest
 *   opposing candidate; if triggered, initiates state 0x0C / sub-state 0x20.
 *
 * Arguments:
 *   actor (unsigned char *): pointer to 1000-byte player actor struct
 *
 * Return value:
 *   int: 1 if interception/block triggered, 0 otherwise
 *
 * Scratchpad / Actor / Global fields read & written:
 *   SCRATCH 0x040/0x070 (s16, R): ball predicted x/z
 *   SCRATCH 0x058 (s16, R): ball predicted y (>= -0xA0 required)
 *   SCRATCH 0x0F8/0x0FC (s16, R/W): target x/z (restored if aborted)
 *   SCRATCH 0x2A9 (u8,  R): current ball owner (must be 0 = free)
 *   SCRATCH 0x2F4 (ptr, R): ball actor pointer (+0x3A0 speed, +0x3A4 heading)
 *   SCRATCH 0x31C (s16, R): ball timer (>= 0x11 required)
 *   SCRATCH 0x31E[2] (u8[2], R): nearest player ID per direction
 *   D_8010C330[side*264 + slot*12] (u32, R): bit 4 = preferred foot
 *   Actor +0x396 (u8, W): set to 0x0C
 *   Actor +0x397 (u8, W): set to 0x20
 *   Actor +0x3AA/+0x3AB (u8, R/W): body/target heading
 *   Actor +0x3AC (u8, W): foot flag from D_8010C330
 *   Actor +0x3C2/+0x3C4/+0x3C6/+0x3C8/+0x3CA/+0x3CC/+0x3D1: action parameters
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010C330[];

extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern void         func_8004840C(unsigned char *actor);
extern void         func_8005BA30(unsigned char *actor, unsigned int anim_id, unsigned int arg2);
extern unsigned int func_800B1128(unsigned int val);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);

int func_800552F4(unsigned char *actor) {
    unsigned int ang_to_ball;
    unsigned char *ball;
    unsigned char ball_heading;
    unsigned int ang_diff;
    signed short saved_x;
    signed short saved_z;
    int dx, dz;
    unsigned int dist_sq;
    unsigned int opp_idx;
    unsigned char *opp;
    unsigned int opp_dist_sq;
    unsigned int stat_off;
    unsigned int dist;
    signed short dist_div5;
    unsigned int new_h;

    if (B(SCRATCH, 0x2A9) != 0) {
        return 0;
    }
    if (S(SCRATCH, 0x31C) < 0x11) {
        return 0;
    }

    ang_to_ball = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(SCRATCH, 0x040), S(SCRATCH, 0x070));
    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    ball_heading = B(ball, 0x3A4);
    if (((ang_to_ball - ball_heading) & 0xFFu) < 0x81u) {
        ang_diff = ang_to_ball - ball_heading;
    } else {
        ang_diff = ball_heading - ang_to_ball;
    }

    saved_x = S(SCRATCH, 0x0F8);
    saved_z = S(SCRATCH, 0x0FC);
    dx = S(actor, 0x02) - saved_x;
    dz = S(actor, 0x06) - saved_z;
    dist_sq = (unsigned int)(dx * dx + dz * dz);

    if (dist_sq > 0x40000u) {
        return 0;
    }
    if (S(SCRATCH, 0x058) < -0xA0) {
        return 0;
    }
    if (W(ball, 0x3A0) < 0x5800u) {
        return 0;
    }
    if (((ang_diff - 0x20u) & 0xFFu) >= 0x41u) {
        return 0;
    }
    if (dist_sq <= 0x8FFFu) {
        return 0;
    }

    /*
     * UNCLEAR: asm tests ((unsigned)(dist_sq + 0x400000) < dist_sq) and returns 0
     * if false; since dist_sq <= 0x40000 was already checked above, this condition
     * always evaluates false and returns 0 at runtime. Preserved verbatim.
     */
    if ((unsigned int)(dist_sq + 0x400000u) >= dist_sq) {
        return 0;
    }

    opp_idx = B(SCRATCH, 0x31E + ((B(actor, 0x399) ^ 1u) << 1));
    opp = D_800FF748 + (opp_idx % 24u) * 0x3E8u;
    func_8004840C(opp);

    dx = S(opp, 0x02) - S(SCRATCH, 0x0F8);
    dz = S(opp, 0x06) - S(SCRATCH, 0x0FC);
    opp_dist_sq = (unsigned int)(dx * dx + dz * dz);
    if (dist_sq < opp_dist_sq) {
        S(SCRATCH, 0x0F8) = saved_x;
        S(SCRATCH, 0x0FC) = saved_z;
        return 0;
    }

    B(actor, 0x3D1) = 0;
    S(actor, 0x3C2) = 0;
    S(actor, 0x3C4) = 1;
    S(actor, 0x3C8) = 0;

    stat_off = (unsigned int)B(actor, 0x392) * 12u + (unsigned int)B(actor, 0x398) * 264u;
    B(actor, 0x3AC) = (unsigned char)((W(D_8010C330, stat_off) >> 4) & 1u);

    func_8005BA30(actor, 0x0B, 0);

    dist = func_800B1128(dist_sq);
    dist_div5 = (signed short)(dist / 5u);
    S(actor, 0x3C6) = dist_div5;
    if (dist_div5 >= 0x39) {
        S(actor, 0x3C6) = 0x38;
    }

    new_h = func_8005B7CC(ang_to_ball & 0xFFu, B(actor, 0x3AA), 0x28);
    B(actor, 0x3AA) = (unsigned char)new_h;
    S(actor, 0x3CA) = 0x0C;
    B(actor, 0x396) = 0x0C;
    B(actor, 0x3AB) = (unsigned char)ang_to_ball;
    S(actor, 0x3CC) = 1;
    B(actor, 0x397) = 0x20;
    return 1;
}
