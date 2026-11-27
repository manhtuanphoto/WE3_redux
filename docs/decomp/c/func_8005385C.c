/*
 * func_8005385C
 *
 * Purpose:
 *   Checks whether a player actor can reach / intercept / control the ball at
 *   predicted ball step `t` (index into Scratchpad predicted-ball trajectory
 *   arrays at 0x36/0x4E/0x66). Performs pitch-boundary and ball-ownership checks
 *   first (resetting actor state to 0x18 / substate 0 on out-of-bounds or invalid
 *   play), then verifies ball height (>= -0x80), ball-lock flag, horizontal
 *   reach radius ((+0x3B4 * t + 0xA0)^2), priority via func_8004968C, and
 *   facing-angle tolerance (< 0x21 = ~46.4 deg) via func_8005B36C.
 *
 * Arguments:
 *   actor (unsigned char *) - pointer to 1000-byte player actor struct
 *   t     (unsigned char)   - lookahead time step index (used as `t & 0xFF`)
 *
 * Return value:
 *   int - 1 if the actor can take/reach the ball at step `t`, 0 otherwise.
 *
 * Actor fields read/written:
 *   +0x02 (s16) - player X position (250 raw = 1 m)
 *   +0x06 (s16) - player Z position (250 raw = 1 m)
 *   +0x38D (u8) - actor ID
 *   +0x396 (u8) - actor state (set to 0x18 on failure before 0x3968)
 *   +0x397 (u8) - actor substate (cleared to 0 on failure before 0x3968)
 *   +0x3AA (u8) - current facing angle (0..255)
 *   +0x3B4 (u8) - speed / reach multiplier
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x36[t] (s16) - predicted ball X at step t
 *   +0x4E[t] (s16) - predicted ball Y at step t (< -0x80 = too high)
 *   +0x66[t] (s16) - predicted ball Z at step t
 *   +0x29A   (u8)  - match phase (1 = open play / kickoff?, 7 = dead/out?)
 *   +0x2A8   (u8)  - last-touch actor ID
 *   +0x2A9   (u8)  - current ball owner actor ID
 *   +0x2F4   (ptr) - ball actor pointer (+0x3B0 u8 checked != 0)
 *   +0x2FC   (u8)  - play-stop / whistle flag
 *
 * Magic numbers (250 raw = 1 m):
 *   0x190000 = 1280^2 (5.12 m radius squared from owned ball)
 *   0x3421   = 13345 raw (53.38 m pitch X limit)
 *   0x2101   = 8449 raw (33.796 m pitch Z limit)
 *   0x4000   = 128^2 (0.512 m close-ball auto-success when phase != 1)
 *   0xA0     = 160 raw (0.64 m base reach radius)
 *   -0x80    = -128 raw (-0.512 m height ceiling; negative Y is up)
 *   0x21     = 33/256 (~46.4 deg half-cone facing tolerance)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCHPAD ((unsigned char *)0x1F800000)

extern unsigned int func_8004968C(unsigned char *actor);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);

int func_8005385C(unsigned char *actor, unsigned char t)
{
    unsigned char *scratch = SCRATCHPAD;
    unsigned char phase = B(scratch, 0x29A);
    unsigned char owner;
    int px;
    int pz;
    int abs_x;
    int abs_z;
    unsigned int step;
    unsigned int reach;
    unsigned int reach_sq;
    unsigned char *ball;
    int bx;
    int bz;
    int dx;
    int dz;
    unsigned int dist_sq;
    unsigned int ang;
    unsigned char facing;

    if (phase == 7 || B(scratch, 0x2FC) != 0) {
        B(actor, 0x396) = 0x18;
        B(actor, 0x397) = 0;
        return 0;
    }

    owner = B(scratch, 0x2A9);
    if (B(actor, 0x38D) == owner) {
        ball = (unsigned char *)W(scratch, 0x2F4);
        dx = S(actor, 2) - S(ball, 2);
        dz = S(actor, 6) - S(ball, 6);
        if (dx * dx + dz * dz <= 0x190000) {
            goto check_ball;
        }
    }

    abs_x = S(actor, 2);
    if (abs_x < 0) {
        abs_x = -abs_x;
    }
    if (abs_x >= 0x3421) {
        B(actor, 0x396) = 0x18;
        B(actor, 0x397) = 0;
        return 0;
    }

    abs_z = S(actor, 6);
    if (abs_z < 0) {
        abs_z = -abs_z;
    }
    if (abs_z >= 0x2101 || owner != 0 || phase != 1) {
        B(actor, 0x396) = 0x18;
        B(actor, 0x397) = 0;
        return 0;
    }

check_ball:
    if (B(actor, 0x38D) == B(scratch, 0x2A8)) {
        return 1;
    }

    if (B(scratch, 0x29A) != 1) {
        dx = S(actor, 2) - S(scratch, 0x36);
        dz = S(actor, 6) - S(scratch, 0x66);
        if (dx * dx + dz * dz < 0x4000) {
            return 1;
        }
    }

    step = (unsigned int)(t & 0xFF);
    reach = (unsigned int)B(actor, 0x3B4) * step + 0xA0U;
    reach_sq = reach * reach;

    if (S(scratch, 0x4E + step * 2U) < -0x80) {
        return 0;
    }

    ball = (unsigned char *)W(scratch, 0x2F4);
    if (B(ball, 0x3B0) != 0) {
        return 0;
    }

    px = S(actor, 2);
    pz = S(actor, 6);
    bx = S(scratch, 0x36 + step * 2U);
    bz = S(scratch, 0x66 + step * 2U);
    dx = px - bx;
    dz = pz - bz;
    dist_sq = (unsigned int)(dx * dx + dz * dz);
    if (dist_sq >= reach_sq) {
        return 0;
    }

    if (func_8004968C(actor) <= reach_sq) {
        return 0;
    }

    ang = func_8005B36C(px, pz, bx, bz);
    facing = B(actor, 0x3AA);
    if (((ang - facing) & 0xFFU) < 0x81U) {
        ang = func_8005B36C(S(actor, 2), S(actor, 6),
                            S(scratch, 0x36 + step * 2U), S(scratch, 0x66 + step * 2U));
        facing = B(actor, 0x3AA);
        return (signed int)((ang - facing) & 0xFFU) < 0x21;
    } else {
        ang = func_8005B36C(S(actor, 2), S(actor, 6),
                            S(scratch, 0x36 + step * 2U), S(scratch, 0x66 + step * 2U));
        facing = B(actor, 0x3AA);
        return (signed int)((facing - ang) & 0xFFU) < 0x21;
    }
}
