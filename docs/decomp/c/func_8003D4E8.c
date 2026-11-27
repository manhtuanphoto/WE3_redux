/*
 * func_8003D4E8
 *
 * Purpose:
 *   Computes a player's shot aim angle toward the opponent's goal from a
 *   predicted ball/target slot in Scratchpad+0x36/4E/66, testing center and
 *   post-side aim offsets for goalkeeper/defender clearance via func_800533F4
 *   when in shooting range, or adding corner/side jitter near goal.
 *
 * Arguments:
 *   actor     (u8*) - pointer to the 1000-byte player actor struct
 *   targetIdx (u8)  - index into Scratchpad+0x36/4E/66 predicted position arrays
 *
 * Return value:
 *   unsigned int - target heading angle byte (0..255)
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x036 (s16[]) predicted x positions indexed by targetIdx (R)
 *     +0x04E (s16[]) predicted y heights indexed by targetIdx (R)
 *     +0x066 (s16[]) predicted z positions indexed by targetIdx (R)
 *   Actor:
 *     +0x002 (s16) x position (R)
 *     +0x006 (s16) z position (R)
 *     +0x399 (u8)  attack direction flag (0 -> goal at +0x3320, 1 -> -0x3320) (R)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x3320 = 13088 raw = 52.352 m (goal line X)
 *   0x23E3 = 9187 raw = 36.748 m (shooting range X threshold)
 *   -0x13F = -319 raw = 1.276 m height threshold (negative = up)
 *   0x2A0  = 672 raw = 2.688 m (goal post inner aim offset; +/-0x80 = +/-0.512 m jitter)
 *   0x300  = 768 raw = 3.072 m (near-goal-line X threshold)
 *   0x380  = 896 raw = 3.584 m (goal mouth Z half-width threshold)
 *   0x31   = 49/256 (~68.9 deg) forward aim cone threshold
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned int func_8005B61C(int, int, int, int);
extern unsigned int func_8005B36C(int, int, int, int);
extern unsigned int func_800533F4(unsigned char*, unsigned int, unsigned int, int);
extern int func_8005C7A0(int);
extern unsigned int func_8001E6B8(unsigned int);

unsigned int func_8003D4E8(unsigned char* actor, unsigned char targetIdx) {
    int goalX;
    int tx;
    int tz;
    unsigned int baseAngle;
    unsigned int outAngle;
    int actorX;
    unsigned int atkDir;
    int zAim;

    goalX = -0x3320;
    if (B(actor, 0x399) == 0) {
        goalX = 0x3320;
    }

    tx = S(Scratchpad, 0x36 + (targetIdx & 0xFF) * 2);
    tz = S(Scratchpad, 0x66 + (targetIdx & 0xFF) * 2);
    baseAngle = func_8005B61C(tx, tz, (short)goalX, 0);

    atkDir = B(actor, 0x399);
    actorX = S(actor, 2);
    if (atkDir != 0) {
        if (actorX > 0) {
            return baseAngle & 0xFF;
        }
    } else {
        if (actorX < 0) {
            return baseAngle & 0xFF;
        }
    }

    if (B(actor, 0x399) != 0) {
        if (-S(actor, 2) < 0x23E3) {
            goto near_goal_or_fallback;
        }
    } else {
        if (S(actor, 2) < 0x23E3) {
            goto near_goal_or_fallback;
        }
    }

    if (S(Scratchpad, 0x4E + (targetIdx & 0xFF) * 2) >= -0x13F) {
        unsigned int tryAngle;
        int dx;
        int distSq;
        int jit1, jit2, dz1, dz2;

        /* Attempt 1: center of goal (goalX, 0) */
        tryAngle = func_8005B36C((short)tx, (short)tz, (short)goalX, 0);
        dx = (B(actor, 0x399) != 0) ? ((short)tx + 0x3320) : ((short)tx - 0x3320);
        distSq = dx * dx + (short)tz * (short)tz;
        if ((func_800533F4(actor, tryAngle & 0xFF, 0x10, distSq) & 0xFF) == 0) {
            return tryAngle & 0xFF;
        }

        /* Attempt 2: +Z post side (goalX, +(0x2A0 + jitter)) */
        jit1 = func_8005C7A0(0x80);
        tryAngle = func_8005B36C((short)tx, (short)tz, (short)goalX, (short)(jit1 + 0x2A0));
        jit1 = func_8005C7A0(0x80);
        jit2 = func_8005C7A0(0x80);
        dz1 = ((short)tz - 0x2A0) - (short)jit1;
        dz2 = ((short)tz - 0x2A0) - (short)jit2;
        dx = (B(actor, 0x399) != 0) ? ((short)tx + 0x3320) : ((short)tx - 0x3320);
        distSq = dx * dx + dz1 * dz2;
        if ((func_800533F4(actor, tryAngle & 0xFF, 0x10, distSq) & 0xFF) == 0) {
            return tryAngle & 0xFF;
        }

        /* Attempt 3: -Z post side (goalX, -(0x2A0 + jitter)) */
        jit1 = func_8005C7A0(0x80);
        tryAngle = func_8005B36C((short)tx, (short)tz, (short)goalX, (short)(-(jit1 + 0x2A0)));
        jit1 = func_8005C7A0(0x80);
        jit2 = func_8005C7A0(0x80);
        dz1 = (short)tz + ((short)jit1 + 0x2A0);
        dz2 = (short)tz + ((short)jit2 + 0x2A0);
        dx = (B(actor, 0x399) != 0) ? ((short)tx + 0x3320) : ((short)tx - 0x3320);
        distSq = dx * dx + dz1 * dz2;
        if ((func_800533F4(actor, tryAngle & 0xFF, 0x10, distSq) & 0xFF) == 0) {
            return tryAngle & 0xFF;
        }
    }

near_goal_or_fallback:
    {
        int absDx = (short)tx - (short)goalX;
        unsigned int forwardAng;
        unsigned int diff;
        int absTz;

        if (absDx < 0) {
            absDx = -absDx;
        }

        if (absDx >= 0x300) {
            forwardAng = (unsigned int)B(actor, 0x399) << 7;
            diff = (baseAngle - forwardAng) & 0xFF;
            if ((diff < 0x81) ? (diff < 0x31) : (((forwardAng - baseAngle) & 0xFF) < 0x31)) {
                goto aim_corner;
            }
        }

        absTz = (short)tz;
        if (absTz < 0) {
            absTz = -absTz;
        }
        if (absTz < 0x380) {
            outAngle = func_8005B61C((short)tx, (short)tz, (short)goalX, S(actor, 6));
            return outAngle & 0xFF;
        }
        return baseAngle & 0xFF;
    }

aim_corner:
    if (func_8001E6B8(2) != 0) {
        zAim = func_8005C7A0((short)(func_8005C7A0(0x80) + 0x2A0));
    } else {
        if (S(actor, 6) > 0) {
            zAim = func_8005C7A0(0x80) + 0x2A0;
        } else {
            zAim = -(func_8005C7A0(0x80) + 0x2A0);
        }
        if (func_8001E6B8(2) == 0) {
            zAim = -zAim;
        }
    }

    outAngle = func_8005B61C((short)tx, (short)tz, (short)goalX, (short)zAim);
    return outAngle & 0xFF;
}
