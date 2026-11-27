/*
 * func_8006B7F4
 *
 * Purpose:
 *   Adjusts and returns the signed 16-bit coordinate at Scratchpad+0xF8 (likely
 *   defensive/offside line or target X/Z reference) for a given actor/player.
 *   Checks Scratchpad[0x2CF + actor->0x398]; if zero, returns Scratchpad+0xF8
 *   unchanged. Otherwise determines attack/team direction `dir` from actor->0x399
 *   (0 -> +1, non-zero -> -1), clamps Scratchpad+0xF8 when far from target
 *   (*(Scratchpad+0x2F4)+2, threshold > 0xC00) and from opposing team line
 *   Scratchpad[0x30C + (side^1)*2] (threshold > 0x900) to (line - dir * 0xA00),
 *   and if Scratchpad+0x2A9 == 0 offsets it further by -(dir * 0x600).
 *
 * Arguments:
 *   actor (unsigned char *): player/actor structure pointer:
 *                            +0x398 (u8): index into Scratchpad+0x2CF flag array
 *                            +0x399 (u8): side/team index (0 or 1)
 *
 * Return value:
 *   Updated signed 16-bit value at Scratchpad+0xF8.
 *
 * Scratchpad (0x1F800000) fields read/written:
 *   +0x0F8 (signed short, R/W): coordinate adjusted and returned
 *   +0x2A9 (unsigned char, R): guard flag disabling the -dir*0x600 shift when non-zero
 *   +0x2CF (unsigned char[], R): per-actor enable flag indexed by actor+0x398
 *   +0x2F4 (unsigned char *, R): pointer whose +0x02 s16 is compared against +0x0F8
 *   +0x30C (signed short[2], R): per-side line coordinate indexed by (side ^ 1)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define SH(p, o) (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

int func_8006B7F4(unsigned char *actor)
{
    unsigned char idx = B(actor, 0x398);
    unsigned char side;
    signed short dir;
    unsigned char *target;
    signed short cur;
    int diff;

    if (B(Scratchpad, 0x2CF + idx) == 0) {
        return SH(Scratchpad, 0x0F8);
    }

    side = B(actor, 0x399);
    dir = (side == 0) ? 1 : -1;

    target = (unsigned char *)W(Scratchpad, 0x2F4);
    cur = SH(Scratchpad, 0x0F8);
    diff = (int)SH(target, 0x02) - (int)cur;
    if (diff < 0) {
        diff = -diff;
    }

    if (diff >= 0xC01) {
        signed short line_x = SH(Scratchpad, 0x30C + ((side ^ 1u) * 2u));
        int line_diff = (int)cur - (int)line_x;
        if (line_diff < 0) {
            line_diff = -line_diff;
        }
        if (line_diff >= 0x901) {
            /* dir * 5 * 512 = dir * 0xA00 (2560) */
            SH(Scratchpad, 0x0F8) = (signed short)(line_x - (dir * 0xA00));
        }
    }

    if (B(Scratchpad, 0x2A9) == 0) {
        /* dir * 3 * 512 = dir * 0x600 (1536) */
        SH(Scratchpad, 0x0F8) = (signed short)(SH(Scratchpad, 0x0F8) - (dir * 0x600));
    }

    return SH(Scratchpad, 0x0F8);
}
