/*
 * func_80067860
 *
 * Purpose:
 *   Finds the closest and second-closest active players on `team` (0 or 1)
 *   within a squared distance cap `max_dist2` and within a directional cone
 *   centered on `dir` from `(x, z)`. Stores both actor IDs in Scratchpad
 *   +0x322 + team*2 (closest) and +0x323 + team*2 (second closest), and
 *   returns the closest actor ID (or 0xFF if none).
 *
 * Arguments:
 *   team         (u8):  team index (0 or 1)
 *   x            (s16): reference X position
 *   z            (s16): reference Z position
 *   dir          (u8):  reference angle (0..255)
 *   max_ang_diff (u8):  angular tolerance used when dist2 > 0x3FFFF
 *   max_dist2    (u32): maximum squared distance
 *
 * Return value:
 *   Closest matching actor ID (u8), or 0xFF if none found.
 *
 * Global fields read:
 *   D_800FF748 (actor table base, 1000-byte / 0x3E8 stride; team 0 at +0x3E8, team 1 at +0x2EE0)
 *
 * Actor fields read:
 *   +0x000 (u8)  active flag
 *   +0x002 (s16) X position
 *   +0x006 (s16) Z position
 *   +0x38D (u8)  actor ID
 *
 * Scratchpad (0x1F800000) fields read/written:
 *   +0x2A9 (u8) ball holder / excluded actor ID (R)
 *   +0x322 + team*2 (u8) closest actor ID (W, init 0xFF)
 *   +0x323 + team*2 (u8) second-closest actor ID (W, init 0xFF)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];

extern unsigned int func_8005B36C(signed short x1, signed short z1, signed short x2, signed short z2);

unsigned char func_80067860(unsigned char team, signed short x, signed short z,
                            unsigned char dir, unsigned char max_ang_diff,
                            unsigned int max_dist2)
{
    unsigned char *actor = (team == 0)
        ? (D_800FF748 + 0x3E8)
        : (D_800FF748 + 0x2EE0);
    unsigned int slot_off = (unsigned int)team << 1;
    unsigned int best_dist2 = 0xFFFFFFFFU;
    unsigned int second_dist2 = 0xFFFFFFFFU;
    int i;

    B(Scratchpad, 0x322 + slot_off) = 0xFF;
    B(Scratchpad, 0x323 + slot_off) = 0xFF;

    for (i = 0; i < 11; i++, actor += 0x3E8) {
        signed int dx, dz;
        unsigned int dist2;
        unsigned int ang;
        unsigned int ang_diff;

        if (B(actor, 0) == 0) {
            continue;
        }
        if (B(actor, 0x38D) == B(Scratchpad, 0x2A9)) {
            continue;
        }

        dx = S(actor, 2) - x;
        dz = S(actor, 6) - z;
        dist2 = (unsigned int)(dx * dx + dz * dz);
        if (max_dist2 < dist2) {
            continue;
        }

        ang = func_8005B36C(x, z, S(actor, 2), S(actor, 6));
        if (((ang - dir) & 0xFFU) < 0x81U) {
            ang = func_8005B36C(x, z, S(actor, 2), S(actor, 6));
            ang_diff = (ang - dir) & 0xFFU;
        } else {
            ang = func_8005B36C(x, z, S(actor, 2), S(actor, 6));
            ang_diff = (dir - ang) & 0xFFU;
        }

        if (dist2 <= 0x3FFFFU) {
            if (ang_diff >= 0x20U) {
                continue;
            }
        } else {
            if (ang_diff > (unsigned int)max_ang_diff) {
                continue;
            }
        }

        if (dist2 <= best_dist2) {
            second_dist2 = best_dist2;
            B(Scratchpad, 0x323 + slot_off) = B(Scratchpad, 0x322 + slot_off);
            best_dist2 = dist2;
            B(Scratchpad, 0x322 + slot_off) = B(actor, 0x38D);
        } else if (dist2 <= second_dist2) {
            second_dist2 = dist2;
            B(Scratchpad, 0x323 + slot_off) = B(actor, 0x38D);
        }
    }

    return B(Scratchpad, 0x322 + slot_off);
}
