/*
 * func_800A8B24
 *
 * Purpose:
 *   Goal-mouth bounds check when the ball crosses the goal line.
 *   Returns 0 immediately if Scratchpad+0xE0 is 2 or 4. Otherwise reads the
 *   ball actor pointer at Scratchpad+0x2F4 and checks whether |ball.z| <= 0x372
 *   (882 raw units, ~3.53 m half-goal-width) and |ball.y| < 0x287 (647 raw units,
 *   ~2.59 m crossbar height).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   signed int: 1 if the ball is within the goal mouth Z/Y bounds and Scratchpad+0xE0
 *               is neither 2 nor 4; 0 otherwise.
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0xE0  (signed short): ball/trajectory block/mode flag (returns 0 if 2 or 4)
 *   +0x2F4 (unsigned char *): pointer to ball actor struct:
 *     +0x04 (signed short): ball Y (height)
 *     +0x06 (signed short): ball Z (lateral position across goal mouth)
 */

#define S(p, o) (*(signed short *)((unsigned char *)(p) + (o)))
#define W(p, o) (*(unsigned int *)((unsigned char *)(p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

signed int func_800A8B24(void)
{
    signed short mode;
    unsigned char *ball;
    signed int abs_z;
    signed int abs_y;

    mode = S(Scratchpad, 0xE0);
    if (mode == 2 || mode == 4) {
        return 0;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);

    abs_z = S(ball, 0x06);
    if (abs_z < 0) {
        abs_z = -abs_z;
    }
    if (abs_z > 0x372) {
        return 0;
    }

    abs_y = S(ball, 0x04);
    if (abs_y < 0) {
        abs_y = -abs_y;
    }

    return (abs_y < 0x287) ? 1 : 0;
}
