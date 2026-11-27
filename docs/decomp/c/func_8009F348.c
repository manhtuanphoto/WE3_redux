/*
 * func_8009F348
 *
 * Purpose:
 *   Computes an 8-bit polar angle (0..255) from Scratchpad (0x1FC, 0x204) to
 *   (0x1F0, 0x1F8) using a two-stage (coarse 16-step by 16, fine 16-step by 1)
 *   Manhattan-error search against sin/cos projections (func_8001E694,
 *   func_8001E5A4) scaled by the Euclidean distance (func_800B1128).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   Best-fit 8-bit angle (0..255)
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x1F0 (s32) target x
 *   +0x1F8 (s32) target z
 *   +0x1FC (s32) origin x
 *   +0x204 (s32) origin z
 */

#define SW(p, o) (*(int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern int func_800B1128(int dist_sq);
extern int func_8001E694(unsigned int angle, int radius);
extern int func_8001E5A4(unsigned int angle, int radius);

unsigned int func_8009F348(void)
{
    int dx = SW(Scratchpad, 0x1F0) - SW(Scratchpad, 0x1FC);
    int dz = SW(Scratchpad, 0x1F8) - SW(Scratchpad, 0x204);
    int dist = func_800B1128(dx * dx + dz * dz);
    unsigned int best_err = 0xFFFFFFFFU;
    int best_angle = 0;
    int angle;
    int step;

    angle = 0;
    for (step = 0; step < 16; step++, angle += 16) {
        unsigned int ang = (unsigned int)angle & 0xFF;
        int px = SW(Scratchpad, 0x1FC) + func_8001E694(ang, dist);
        int ex = SW(Scratchpad, 0x1F0) - px;
        int pz = SW(Scratchpad, 0x204) + func_8001E5A4(ang, dist);
        int ez = SW(Scratchpad, 0x1F8) - pz;
        unsigned int err;

        if (ex < 0) {
            ex = -ex;
        }
        if (ez < 0) {
            ez = -ez;
        }
        err = (unsigned int)(ex + ez);
        if (err < best_err) {
            best_err = err;
            best_angle = angle;
        }
    }

    best_err = 0xFFFFFFFFU;
    angle = best_angle - 8;
    for (step = 0; step < 16; step++, angle += 1) {
        unsigned int ang = (unsigned int)angle & 0xFF;
        int px = SW(Scratchpad, 0x1FC) + func_8001E694(ang, dist);
        int ex = SW(Scratchpad, 0x1F0) - px;
        int pz = SW(Scratchpad, 0x204) + func_8001E5A4(ang, dist);
        int ez = SW(Scratchpad, 0x1F8) - pz;
        unsigned int err;

        if (ex < 0) {
            ex = -ex;
        }
        if (ez < 0) {
            ez = -ez;
        }
        err = (unsigned int)(ex + ez);
        if (err < best_err) {
            best_err = err;
            best_angle = angle;
        }
    }

    return (unsigned int)best_angle & 0xFF;
}
