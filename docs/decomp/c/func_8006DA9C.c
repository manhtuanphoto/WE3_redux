/*
 * func_8006DA9C
 *
 * Purpose:
 *   Checks whether the actor/object referenced by Scratchpad+0x2F4 is facing
 *   within +-24 angular units (< 0x19 out of 256) of `actorB`'s team direction
 *   (`(actorB+0x399) * 128`), AND whether `pos` (signed 16-bit) lies beyond a
 *   team-dependent threshold (`Scratchpad+0x314[actorA+0x399] - 0x400`):
 *   - For `actorA+0x399 == 0`: `pos < threshold`.
 *   - For `actorA+0x399 != 0`: `-pos < -threshold` (i.e. `pos > threshold`).
 *
 * Arguments:
 *   actorA (unsigned char *) - actor struct (+0x399 u8 team side for position threshold)
 *   actorB (unsigned char *) - actor struct (+0x399 u8 team side for facing check)
 *   pos    (signed short)    - coordinate compared against team threshold
 *
 * Return value:
 *   (int) 1 if both angle and position checks pass, 0 otherwise.
 *
 * Actor / Scratchpad (0x1F800000) fields read:
 *   Scratchpad+0x2F4 (u8 *)    pointer to object whose +0x3A4 (u8) is heading (0..255)
 *   Scratchpad+0x314 (s32[2])  per-team threshold base (offset -0x400 applied)
 *   actorA+0x399     (u8)      team index (0 or 1)
 *   actorB+0x399     (u8)      team index (0 or 1; << 7 = 0 or 128 target heading)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

int func_8006DA9C(unsigned char *actorA, unsigned char *actorB, signed short pos)
{
    unsigned char *obj = *(unsigned char **)(Scratchpad + 0x2F4);
    unsigned int targetAngle = (unsigned int)B(actorB, 0x399) << 7;
    unsigned int objAngle = B(obj, 0x3A4);
    unsigned int diff = (targetAngle - objAngle) & 0xFFU;
    unsigned int sideA;
    int threshold;
    int sPos;

    if (diff < 0x81U) {
        if ((int)diff >= 0x19) {
            return 0;
        }
    } else {
        if ((int)((objAngle - targetAngle) & 0xFFU) >= 0x19) {
            return 0;
        }
    }

    sideA = B(actorA, 0x399);
    threshold = SW(Scratchpad, 0x314 + (sideA << 2)) - 0x400;
    sPos = (int)pos;

    if (sideA != 0U) {
        return (-sPos < -threshold) ? 1 : 0;
    }
    return (sPos < threshold) ? 1 : 0;
}
