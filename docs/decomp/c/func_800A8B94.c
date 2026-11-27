/*
 * func_800A8B94
 *
 * Purpose:
 *   Checks ball position, height, velocity, and heading when past the goal line
 *   (|ball->x| >= 0x3321) to detect and resolve goal-net / crossbar / goalpost
 *   collisions and goal-box containment. Updates Scratchpad+0xE0, Scratchpad+0xC2,
 *   and D_800FF6A4, or calls func_80098968 / func_800987CC.
 *
 * Arguments:
 *   None
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x0C2 (s16) set to ball->vz / 6 when ball enters back-of-net clamp (W)
 *     +0x0E0 (s16) goal/net event state; skips if != 0, set to 2 or 5 (R/W)
 *     +0x2F4 (ptr) ball entity (+0x02 x, +0x04 y, +0x06 z, +0x0C vy, +0x10 vz,
 *                  +0x3A0 speed, +0x3A4 dir, +0x3C2 prev_x, +0x3C4 prev_y, +0x3C6 prev_z) (R/W)
 *     +0x2FC (u8)  flag skipping goal/net check when == 1 (R)
 *   D_800FF6A4: quadrant byte (1..4) on goalpost/side-netting hit (W)
 *
 * Magic numbers:
 *   0x3321 (13089): goal line X threshold (goal line at 0x3320)
 *   0x35C5 (13765), 0x3542 (13634): back-of-net X bounds
 *   0x372 (882), 0x392 (914), 0x3F2 (1010), 0x302 (770): goal mouth Z bounds
 *   -0x2D1 (-721), -0x28D (-653), -0x265 (-613): crossbar / net Y heights
 *   0x9A00, 0xB000: ball speed thresholds (+0x3A0)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define P(p,o)  (*(unsigned char**)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF6A4;

extern void func_80098968(void);
extern void func_800987CC(unsigned char *ball, int mode);

static int is_angle_near_axis(unsigned char ang, int tol) {
    unsigned char d0 = (unsigned char)(-ang);
    unsigned char d1;
    int ok0 = (d0 < 0x81) ? (d0 < tol) : ((signed char)ang < tol);
    if (ok0) {
        return 1;
    }
    d1 = (unsigned char)(0x80 - ang);
    return (d1 < 0x81) ? (d1 < tol) : (((ang + 0x80) & 0xFF) < tol);
}

void func_800A8B94(void) {
    unsigned char *ball;
    signed short bx;
    signed short by;
    signed short bz;
    int abs_x;
    int abs_z;
    int tol_z_adj;

    if (B(Scratchpad, 0x2FC) == 1 || S(Scratchpad, 0x0E0) != 0) {
        return;
    }

    ball = P(Scratchpad, 0x2F4);
    bx = S(ball, 0x02);
    abs_x = (bx >= 0) ? bx : -bx;
    if (abs_x < 0x3321) {
        return;
    }

    by = S(ball, 0x04);
    bz = S(ball, 0x06);
    abs_z = (bz >= 0) ? bz : -bz;

    if ((unsigned short)(by + 0x2D1) < 0xD7U &&
        S(ball, 0x3C4) < by &&
        abs_x < 0x35C5 &&
        abs_z < 0x372 &&
        SW(ball, 0x0C) >= 0x81) {
        int allow = 1;
        if (is_angle_near_axis(B(ball, 0x3A4), 0x28)) {
            if (W(ball, 0x3A0) > 0x9A00U) {
                allow = 0;
            }
        }
        if (allow && W(ball, 0x3A0) <= 0xB000U) {
            S(Scratchpad, 0x0E0) = 2;
            func_80098968();
            return;
        }
    }

    tol_z_adj = is_angle_near_axis(B(ball, 0x3A4), 0x20) ? 0 : 0x60;

    if (abs_x < 0x35C5 &&
        abs_z < (0x3F2 + tol_z_adj) &&
        (0x302 - tol_z_adj) < abs_z) {
        signed short prev_z = S(ball, 0x3C6);
        int abs_prev_z = (prev_z >= 0) ? prev_z : -prev_z;

        if (abs_z < abs_prev_z && by >= -0x28D) {
            unsigned char quadrant;
            if (bx > 0) {
                quadrant = (bz > 0) ? 1 : 3;
            } else {
                quadrant = (bz > 0) ? 2 : 4;
            }
            D_800FF6A4 = quadrant;
            func_800987CC(ball, 1);
            return;
        }
    }

    if (by >= -0x265 && abs_z < 0x392 && abs_x < 0x3543) {
        signed short prev_x = S(ball, 0x3C2);
        int abs_prev_x = (prev_x >= 0) ? prev_x : -prev_x;
        if (abs_prev_x >= 0x3543) {
            S(ball, 0x02) = (bx >= 0) ? 0x3542 : -0x3542;
            S(Scratchpad, 0x0E0) = 5;
            S(Scratchpad, 0x0C2) = (signed short)(SW(ball, 0x10) / 6);
        }
    }
}
