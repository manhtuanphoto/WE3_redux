/*
 * func_800676C4
 *
 * Purpose:
 *   Scans a team's active players in D_800FF748 (stride 0x3E8) to find the
 *   closest and second-closest players to target world position (targetX, targetZ)
 *   using a 4-tick lookahead position (pos + vel * 4), and writes their player
 *   IDs (+0x38D) to Scratchpad+0x322 + (team * 2) and +0x323 + (team * 2).
 *
 * Arguments:
 *   team:       (unsigned char) team index (0 = home, nonzero = away)
 *   targetX:    (signed short) target X coordinate
 *   targetZ:    (signed short) target Z coordinate
 *   startOff:   (unsigned char) starting offset/flag (0 = skip GK, 1 = include GK)
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x322 + team*2 (u8) closest player ID (W)
 *     +0x323 + team*2 (u8) second-closest player ID (W)
 *   D_800FF748: array of player structs (stride 0x3E8 = 1000 bytes):
 *     +0x000 (u8)  active flag (0 = skip) (R)
 *     +0x002 (s16) X position (R)
 *     +0x006 (s16) Z position (R)
 *     +0x008 (s32) X velocity (R)
 *     +0x010 (s32) Z velocity (R)
 *     +0x38D (u8)  player ID (R)
 *     +0x396 (u8)  player state (0x14 and 0x35 are skipped) (R)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];

void func_800676C4(unsigned char team, signed short targetX, signed short targetZ, unsigned char startOff) {
    unsigned int rem = (unsigned int)startOff % 24U;
    int startIdx;
    int count;
    unsigned char *player;
    unsigned int bestDist = 0xFFFFFFFFU;
    unsigned int secondDist = 0xFFFFFFFFU;
    unsigned char *outPair;
    int i;

    if (team == 0) {
        startIdx = 2 - (int)rem;
    } else {
        startIdx = 13 - (int)rem;
    }

    player = D_800FF748 + (startIdx * 0x3E8);
    count = (int)startOff + 10;
    if (count <= 0) {
        return;
    }

    outPair = Scratchpad + 0x322 + ((unsigned int)team << 1);

    for (i = 0; i < count; i++, player += 0x3E8) {
        unsigned char state;
        int dx, dz;
        unsigned int distSq;

        if (B(player, 0x000) == 0) {
            continue;
        }

        state = B(player, 0x396);
        if (state == 0x14 || state == 0x35) {
            continue;
        }

        dx = (S(player, 0x002) + (SW(player, 0x008) << 2)) - targetX;
        dz = (S(player, 0x006) + (SW(player, 0x010) << 2)) - targetZ;
        distSq = (unsigned int)(dx * dx + dz * dz);

        if (distSq <= bestDist) {
            secondDist = bestDist;
            outPair[1] = outPair[0];
            bestDist = distSq;
            outPair[0] = B(player, 0x38D);
        } else if (distSq <= secondDist) {
            secondDist = distSq;
            outPair[1] = B(player, 0x38D);
        }
    }
}
