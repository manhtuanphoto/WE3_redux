/*
 * func_80053D34
 *
 * Purpose:
 *   Checks whether a player can intercept or reach the predicted ball position
 *   within frames 1..7. Verifies that the player's facing angle (+0x3A4) is
 *   within 0x40 (90 degrees) of the target actor at Scratchpad+0x2F4, then
 *   scans frames t = 1..7 of the Scratchpad predicted ball trajectory
 *   (X at +0x34+2t, Z at +0x4C+2t, Y at +0x64+2t) to see if the ball's Z height
 *   falls inside a stat-scaled window from D_8010C330 and horizontal squared
 *   distance from the player's projected position is <= 0x8FFF.
 *
 * Arguments:
 *   player (unsigned char *): pointer to player struct
 *
 * Return value:
 *   1 if an interception frame (1..7) satisfies angular, height, and distance checks;
 *   0 otherwise.
 *
 * Reads:
 *   Scratchpad (0x1F800000):
 *     +0x2F4 (unsigned char *): reference/target player pointer (+0x3A4 facing angle)
 *     +0x036, +0x038, ... (+0x034 + 2*t, s16): predicted ball X for frame t=1..7
 *     +0x04E, +0x050, ... (+0x04C + 2*t, s16): predicted ball Z (height) for frame t=1..7
 *     +0x066, +0x068, ... (+0x064 + 2*t, s16): predicted ball Y for frame t=1..7
 *   player:
 *     +0x002 (s16): X position
 *     +0x006 (s16): Y position
 *     +0x008 (s32): X velocity
 *     +0x010 (s32): Y velocity
 *     +0x392 (u8): player slot index (stride 12 in team block)
 *     +0x398 (u8): team index (stride 264 = 22 * 12 in D_8010C330)
 *     +0x3A4 (u8): facing angle (0..255)
 *   D_8010C330: player stats table (bits 11..17 extracted via (word >> 11) & 0x7F)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_8010C330[];

signed int func_80053D34(unsigned char *player)
{
    unsigned char *target = *(unsigned char **)(Scratchpad + 0x2F4);
    unsigned int playerAngle = B(player, 0x3A4);
    unsigned int targetAngle = B(target, 0x3A4);
    unsigned int diff = (targetAngle - playerAngle) & 0xFF;
    signed int statVal;
    signed int minZ;
    signed int maxZ;
    signed int t;
    unsigned char *spCursor;

    if (diff < 0x81) {
        if ((signed int)diff >= 0x40) {
            return 0;
        }
    } else {
        if ((signed int)((playerAngle - targetAngle) & 0xFF) >= 0x40) {
            return 0;
        }
    }

    /* Stat lookup: D_8010C330 + team * 264 + slot * 12 */
    {
        unsigned int slotOff = (unsigned int)B(player, 0x392) * 12U;
        unsigned int teamOff = (unsigned int)B(player, 0x398) * 264U;
        unsigned int statWord = W(D_8010C330, teamOff + slotOff);
        statVal = (signed int)((statWord >> 11) & 0x7F) + 0x64;
    }

    /* Signed divisions by 180 via fixed-point reciprocal 0xB60B60B7 */
    minZ = (-statVal * 512) / 180;
    maxZ = (-statVal * 320) / 180;

    spCursor = Scratchpad + 2;
    for (t = 1; t < 8; t++, spCursor += 2) {
        signed int ballZ = S(spCursor, 0x4C);
        if (ballZ > minZ && ballZ < maxZ) {
            signed int predX = S(player, 0x02) + SW(player, 0x08) * t - S(spCursor, 0x34);
            signed int predY = S(player, 0x06) + SW(player, 0x10) * t - S(spCursor, 0x64);
            signed int distSq = predX * predX + predY * predY;
            if (distSq <= 0x8FFF) {
                return 1;
            }
        }
    }

    return 0;
}
