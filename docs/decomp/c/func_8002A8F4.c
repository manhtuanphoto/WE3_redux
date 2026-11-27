/*
 * func_8002A8F4
 * Purpose: Selects a shot aim heading (0..255 byte angle) and target Z offset
 *          (+0x3C4) toward the opponent's goal, checking line-of-sight/goalkeeper
 *          blockage via func_800533F4, CPU heuristics, or human directional input.
 * Arguments:
 *   - actor (unsigned char *): pointer to 1000-byte shooter actor struct
 * Return value: unsigned int (0..255 byte angle toward chosen goal target)
 * Actor fields read/written:
 *   - +0x02 (s16): X position (250 raw = 1 m)
 *   - +0x06 (s16): Z position (250 raw = 1 m)
 *   - +0x399 (u8): attack direction flag (0 = attacking +X / +0x3300, 1 = -X / -0x3300)
 *   - +0x39A (u8): CPU/AI flag (0 = human, nonzero = CPU)
 *   - +0x3AA (u8): body heading byte
 *   - +0x3C2 (s16): target/contact flag (0 checked for corner preference)
 *   - +0x3C4 (s16): written with chosen goal-mouth target Z coordinate
 *   - +0x3D4 (u16): directional input bitmask for human shot aiming
 * Scratchpad fields read:
 *   - 0x1F80028C (u8): directional bitmask when 0x2E6 >= 5 (bits 0x1, 0x4)
 *   - 0x1F8002E1 (u8): match mode (5 = penalty / direct aim skip)
 *   - 0x1F8002E6 (u8): camera/control mode (< 5 uses actor+0x3D4; >= 5 uses scratch+0x28C)
 *   - 0x1F8002E7 (u8): camera angle byte (0x80 flips target Z sign)
 *   - 0x1F8002F4 (ptr): pointer to ball actor (+0x02 X, +0x06 Z)
 * Magic numbers:
 *   - 0x2F21 (12065 raw = 48.26 m): close-to-goal-line X threshold
 *   - 0x3300 (13056 raw = 52.224 m) / 0x3320 (13088 raw = 52.352 m): goal line X
 *   - 0x400 (1024 raw = 4.096 m): goal-mouth Z half-width check
 *   - 0x2A0 (672 raw = 2.688 m) +- 0x80 (128 raw = 0.512 m): inside-post Z aim target
 *   - 0x40 (64/256 = 90 deg) / 0x38 (56/256 = 78.75 deg): body/goal alignment thresholds
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCHPAD ((unsigned char *)0x1F800000)

extern unsigned int func_8005B36C(int, int, int, int);
extern int func_8005C7A0(int);
extern int func_8001E6B8(int);
extern int func_800533F4(unsigned char *, unsigned int, int, int);

unsigned int func_8002A8F4(unsigned char *actor)
{
    unsigned char *scratch = SCRATCHPAD;
    unsigned char *ball = *(unsigned char **)(scratch + 0x2F4);
    int ballX = S(ball, 0x2);
    int goalX;
    unsigned int centerAngle;
    unsigned int bodyHeading;
    unsigned int diff;
    int targetZ;

    if (B(actor, 0x399) != 0) {
        if (-ballX >= 0x2F21) {
            int bz = S(ball, 0x6);
            if ((bz >= 0 ? bz : -bz) < 0x400) {
                S(actor, 0x3C4) = (signed short)bz;
                return (B(actor, 0x399) << 7) & 0x80;
            }
        }
    } else {
        if (ballX >= 0x2F21) {
            int bz = S(ball, 0x6);
            if ((bz >= 0 ? bz : -bz) < 0x400) {
                S(actor, 0x3C4) = (signed short)bz;
                return (B(actor, 0x399) << 7) & 0x80;
            }
        }
    }

    goalX = (B(actor, 0x399) != 0) ? -0x3300 : 0x3300;
    ball = *(unsigned char **)(scratch + 0x2F4);
    centerAngle = func_8005B36C(S(ball, 0x2), S(ball, 0x6), goalX, 0);

    bodyHeading = B(actor, 0x3AA);
    diff = (centerAngle - bodyHeading) & 0xFF;
    if ((diff < 0x81U && (int)diff >= 0x40) ||
        (diff >= 0x81U && (int)((bodyHeading - centerAngle) & 0xFF) >= 0x40)) {
        int az;
        if (B(actor, 0x3AA) < 0x80) {
            targetZ = func_8005C7A0(0x80) + 0x2A0;
        } else {
            targetZ = -(func_8005C7A0(0x80) + 0x2A0);
        }
        az = S(actor, 0x6);
        if ((az >= 0 ? az : -az) >= 0x400 && func_8001E6B8(2) == 0) {
            int z16 = (signed short)targetZ;
            if (S(actor, 0x6) * z16 > 0) {
                int scale = func_8001E6B8(0x10) + 0xE0;
                targetZ = (int)((unsigned int)(z16 * scale) >> 8);
                goto return_angle;
            }
            S(actor, 0x3C4) = (signed short)targetZ;
            ball = *(unsigned char **)(scratch + 0x2F4);
            return func_8005B36C(S(ball, 0x2), S(ball, 0x6), goalX, (signed short)targetZ) & 0xFF;
        }
        goto return_angle;
    }

    {
        unsigned int goalHeading = (unsigned int)B(actor, 0x399) << 7;
        unsigned int gdiff = (centerAngle - goalHeading) & 0xFF;
        if ((gdiff < 0x81U && (int)gdiff >= 0x38) ||
            (gdiff >= 0x81U && (int)((goalHeading - centerAngle) & 0xFF) >= 0x38)) {
            S(actor, 0x3C4) = 0;
            return centerAngle & 0xFF;
        }
    }

    if (B(scratch, 0x2E1) != 5) {
        int ax = S(actor, 0x2);
        int dx = (B(actor, 0x399) != 0) ? (ax + 0x3320) : (ax - 0x3320);
        int distSq = dx * dx + S(actor, 0x6) * S(actor, 0x6);
        unsigned int candAngle;
        int dx2, dz1, dz2;

        if ((func_800533F4(actor, centerAngle & 0xFF, 0xC, distSq) & 0xFF) == 0) {
            S(actor, 0x3C4) = 0;
            return centerAngle & 0xFF;
        }

        targetZ = func_8005C7A0(0x80) + 0x2A0;
        ball = *(unsigned char **)(scratch + 0x2F4);
        candAngle = func_8005B36C(S(ball, 0x2), S(ball, 0x6), goalX, (signed short)targetZ);

        ax = S(actor, 0x2);
        dx2 = (B(actor, 0x399) != 0) ? (ax + 0x3320) : (ax - 0x3320);
        dz1 = S(actor, 0x6) - 0x2A0 - (signed short)func_8005C7A0(0x80);
        dz2 = S(actor, 0x6) - 0x2A0 - (signed short)func_8005C7A0(0x80);
        distSq = dx2 * dx2 + dz1 * dz2;
        if ((func_800533F4(actor, candAngle & 0xFF, 0xC, distSq) & 0xFF) == 0) {
            S(actor, 0x3C4) = (signed short)(func_8005C7A0(0x80) + 0x2A0);
            return candAngle & 0xFF;
        }

        targetZ = -(func_8005C7A0(0x80) + 0x2A0);
        ball = *(unsigned char **)(scratch + 0x2F4);
        candAngle = func_8005B36C(S(ball, 0x2), S(ball, 0x6), goalX, (signed short)targetZ);

        ax = S(actor, 0x2);
        dx2 = (B(actor, 0x399) != 0) ? (ax + 0x3320) : (ax - 0x3320);
        dz1 = S(actor, 0x6) + ((signed short)func_8005C7A0(0x80) + 0x2A0);
        dz2 = S(actor, 0x6) + ((signed short)func_8005C7A0(0x80) + 0x2A0);
        distSq = dx2 * dx2 + dz1 * dz2;
        if ((func_800533F4(actor, candAngle & 0xFF, 0xC, distSq) & 0xFF) == 0) {
            S(actor, 0x3C4) = (signed short)(-(func_8005C7A0(0x80) + 0x2A0));
            return candAngle & 0xFF;
        }
    }

    if (B(actor, 0x39A) != 0) {
        unsigned int r = (unsigned int)func_8001E6B8(9);
        if (r < 9U) {
            switch (r) {
            case 0:
            case 1:
            case 2:
                if (S(actor, 0x6) > 0) {
                    goto aim_pos_post;
                }
                goto aim_neg_post;
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
                if (S(actor, 0x6) > 0) {
                    goto aim_neg_post;
                }
                goto aim_pos_post;
            case 8:
                goto aim_center_jitter;
            }
        }
        goto aim_center_jitter;
    }

    if (B(scratch, 0x2E6) >= 5) {
        unsigned char pad = B(scratch, 0x28C);
        if (pad & 0x1) {
            targetZ = func_8005C7A0(0x80) + 0x2A0;
        } else if (pad & 0x4) {
            targetZ = -(func_8005C7A0(0x80) + 0x2A0);
        } else if (func_8001E6B8(3) != 0 || S(actor, 0x3C2) != 0) {
            int j = (signed short)func_8005C7A0(0x80);
            targetZ = func_8005C7A0((signed short)((j * 2 + 0x540) / 3));
        } else if (func_8001E6B8(2) != 0) {
            targetZ = func_8005C7A0(0x80) + 0x2A0;
        } else {
            targetZ = -(func_8005C7A0(0x80) + 0x2A0);
        }

        if (B(scratch, 0x2E7) == 0x80) {
            targetZ = -(signed short)targetZ;
        }
        goto return_angle;
    }

    if (B(actor, 0x399) == 0) {
        unsigned short mask = H(actor, 0x3D4);
        if (mask & 0x9000) {
            goto aim_pos_post;
        }
        if (mask & 0x6000) {
            goto aim_neg_post;
        }
        if (func_8001E6B8(3) == 0 && S(actor, 0x3C2) == 0) {
            if (func_8001E6B8(2) == 0) {
                goto aim_neg_post;
            }
            goto aim_pos_post;
        }
        goto aim_center_jitter;
    } else {
        unsigned short mask = H(actor, 0x3D4);
        if (mask & 0x3000) {
            goto aim_pos_post;
        }
        if (mask & 0xC000) {
            goto aim_neg_post;
        }
        if (func_8001E6B8(3) == 0 && S(actor, 0x3C2) == 0) {
            if (func_8001E6B8(2) != 0) {
                goto aim_pos_post;
            }
            goto aim_neg_post;
        }
        goto aim_center_jitter;
    }

aim_pos_post:
    targetZ = func_8005C7A0(0x80) + 0x2A0;
    goto return_angle;

aim_neg_post:
    targetZ = -(func_8005C7A0(0x80) + 0x2A0);
    goto return_angle;

aim_center_jitter:
    {
        int j = (signed short)func_8005C7A0(0x80);
        targetZ = func_8005C7A0((signed short)((j * 2 + 0x540) / 3));
    }

return_angle:
    ball = *(unsigned char **)(scratch + 0x2F4);
    S(actor, 0x3C4) = (signed short)targetZ;
    return func_8005B36C(S(ball, 0x2), S(ball, 0x6), goalX, (signed short)targetZ) & 0xFF;
}
