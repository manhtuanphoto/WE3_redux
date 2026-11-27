/*
 * func_80052EBC
 *
 * Purpose:
 *   Determines whether an actor should pursue/contest the ball. When the ball
 *   is free (Scratchpad+0x2A9 == 0), checks mode restrictions (+0x2E2), helper
 *   func_80053EB8, ball height at Scratchpad+0x4E against a stat-scaled reach
 *   ceiling, and whether the actor is within 768 raw units (3.072 m) of the
 *   predicted target (+0xF8/+0xFC) or closer to it than the opposing marked
 *   player (+0x31E). When the ball is owned (+0x2A9 != 0), returns 1 if the
 *   actor is the owner, or if the actor's team has defensive/contest flag
 *   +0x2CF[side] and the actor is within 16 raw units (0.064 m) or nearly as
 *   close to the ball (+0x36/+0x66) as the ball owner.
 *
 * Arguments:
 *   actor (u8 *) - pointer to 1000-byte actor struct
 *
 * Return value:
 *   1 if actor should pursue/contest the ball, 0 otherwise
 *
 * Actor fields read:
 *   +0x02 (s16) x, +0x06 (s16) z, +0x38D (u8) actor id,
 *   +0x392 (u8) team slot, +0x398 (u8) side, +0x399 (u8) attack direction flag
 *
 * Global fields read:
 *   D_800FF748 (actor table), D_8010C330 (player stats table, bits 11..17)
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x36 (s16) ball x, +0x4E (s16) ball y (height, negative = up),
 *   +0x66 (s16) ball z, +0xF8 (s16) predicted target x,
 *   +0xFC (s16) predicted target z, +0x2A9 (u8) ball owner (0 = free),
 *   +0x2AA (u8) previous ball owner, +0x2CF[side] (u8) possession flags,
 *   +0x2E2 (u8) sub-mode, +0x31E[2] (u8) per-side designated chaser actor id
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   -((stat + 100) * 352) / 180 - height ceiling check (for stat >= 0, <= -195 raw = 0.78 m)
 *   0x8FFFF (589823; dist^2 <= 0x8FFFF -> dist < 768 raw = 3.072 m)
 *   0x101 (257; dist^2 < 0x101 -> dist <= 16 raw = 0.064 m)
 *   0x1000 (4096) - owner distance-squared margin when contesting owned ball
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned int D_8010C330[];

extern int func_80053EB8(unsigned char *actor);

int func_80052EBC(unsigned char *actor)
{
    unsigned int owner = B(Scratchpad, 0x2A9);

    if (owner == 0) {
        unsigned int mode2 = B(Scratchpad, 0x2E2);
        unsigned int off;
        int stat;
        int ceiling;
        signed short tx, tz;
        int dx, dz;
        unsigned int dist_sq;
        unsigned int other_id;
        unsigned char *other;
        unsigned int other_dist_sq;

        if (mode2 - 2U < 2U) {
            if (B(actor, 0x38D) != B(Scratchpad, 0x31E + B(actor, 0x399) * 2)) {
                return 0;
            }
        }

        if ((func_80053EB8(actor) & 0xFF) == 0) {
            return 0;
        }

        off = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        stat = (int)(((W((unsigned char *)D_8010C330, off) >> 11) & 0x7F) + 100);
        ceiling = (-(stat * 352)) / 180;
        if (S(Scratchpad, 0x4E) < ceiling) {
            return 0;
        }

        tx = S(Scratchpad, 0xF8);
        tz = S(Scratchpad, 0xFC);
        dx = S(actor, 2) - tx;
        dz = S(actor, 6) - tz;
        dist_sq = (unsigned int)(dx * dx + dz * dz);

        other_id = B(Scratchpad, 0x31E + (B(actor, 0x399) ^ 1) * 2);
        other = D_800FF748 + (other_id % 24) * 1000;

        if (B(actor, 0x38D) == B(Scratchpad, 0x2AA)) {
            return 1;
        }
        if (dist_sq <= 0x8FFFFU) {
            return 1;
        }

        dx = S(other, 2) - tx;
        dz = S(other, 6) - tz;
        other_dist_sq = (unsigned int)(dx * dx + dz * dz);
        if (dist_sq < other_dist_sq) {
            return 1;
        }
        return 0;
    } else {
        unsigned char *owner_actor;
        signed short bx, bz;
        int dx, dz;
        unsigned int dist_sq;
        unsigned int owner_dist_sq;

        if (B(actor, 0x38D) == owner) {
            return 1;
        }
        if (B(Scratchpad + B(actor, 0x398), 0x2CF) == 0) {
            return 0;
        }

        owner_actor = D_800FF748 + (owner % 24) * 1000;
        bx = S(Scratchpad, 0x36);
        bz = S(Scratchpad, 0x66);
        dx = S(actor, 2) - bx;
        dz = S(actor, 6) - bz;
        dist_sq = (unsigned int)(dx * dx + dz * dz);
        if (dist_sq < 0x101U) {
            return 1;
        }

        dx = S(owner_actor, 2) - bx;
        dz = S(owner_actor, 6) - bz;
        owner_dist_sq = (unsigned int)(dx * dx + dz * dz);
        if (owner_dist_sq + 0x1000U < dist_sq) {
            return 0;
        }
        return 1;
    }
}
