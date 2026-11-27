/*
 * func_80063F40
 * Purpose: CPU/AI decision routine when a player is on the ball or in an attacking/
 *          dribbling phase (controlled by D_8010C1ED: 0 = initial/open-space check,
 *          1 = decide between shooting/clearing, passing, or continuing dribble).
 * Arguments:
 *   - actor (unsigned char *): pointer to 1000-byte player actor struct
 * Return value: void
 * Actor fields read/written:
 *   - +0x02 (s16): X position (250 raw = 1 m)
 *   - +0x06 (s16): Z position (250 raw = 1 m)
 *   - +0x38D (u8): actor ID
 *   - +0x392 (u8): team slot (0..10)
 *   - +0x396 (u8): actor state (early return if == 2; checked for == 3)
 *   - +0x398 (u8): team side (0/1)
 *   - +0x399 (u8): attack direction flag (0 = +X goal, 1 = -X goal)
 *   - +0x3AA (u8): body heading
 * Scratchpad reads:
 *   - 0x1F8002F4 (ptr): pointer to ball actor (+0x3A8 = ball heading)
 *   - 0x1F80031E[oppSide] (u8): nearest opponent actor ID
 * Globals read/written:
 *   - D_8010C1ED (u8): AI dribble/action sub-mode (0 or 1)
 *   - D_8010C1D8 (struct): +0x00 (u8) mode/lock, +0x02 (u8) pass target id,
 *                          +0x14 (u8) action code (2 = pass), +0x15 (u8) timer/counter
 *   - D_8010C328 / D_8010C32C / D_8010C32E: player stat words indexed by side*264 + slot*12
 *   - D_800E6CB2 (u8): difficulty/threshold multiplier
 *   - D_800FF748 (actor[24]): 1000-byte actor array
 *   - D_800C84A8 / D_800C84B0: direction candidate tables per attack side
 * Magic numbers (250 raw = 1 m):
 *   - 0x18 (24/256 = 33.75 deg), 0x20 (45 deg), 0x22 (47.8 deg), 0x31 (68.9 deg): heading tolerances
 *   - 0x3FFFF (512^2 = 2.048^2 m^2): close opponent distance squared threshold
 *   - 0x310000 (1792^2 = 7.168^2 m^2): base distance squared threshold for nearby opponent
 *   - 0x27E3 (10211 = 40.84 m), 0x22E3 (8931 = 35.72 m), 0x1CE3 (7395 = 29.58 m),
 *     0x2E09 (11785 = 47.14 m), 0x2F21 (12065 = 48.26 m): X-axis field/box thresholds
 *   - 0x8B6 (2230 = 8.92 m), 0x9B6 (2486 = 9.94 m), 0xCF4 (3316 = 13.26 m): Z-axis box/corridor half-widths
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCHPAD ((unsigned char *)0x1F800000)

extern unsigned char D_8010C1D8[];
extern unsigned char D_8010C1ED[];
extern unsigned char D_800FF748[];
extern unsigned char D_800C84A8[];
extern unsigned char D_800C84B0[];
extern unsigned char D_8010C328[];
extern unsigned char D_8010C32C[];
extern unsigned char D_8010C32E[];
extern unsigned char D_800E6CB2[];

extern void func_800667E8(unsigned char *, int);
extern void func_80064B44(unsigned char *);
extern int func_800653D4(unsigned char *, int, int);
extern void func_8005E09C(unsigned char *, unsigned int, int);
extern unsigned int func_8005CE30(unsigned int);
extern int func_8001E6B8(int);
extern int func_800652EC(unsigned char *, int);
extern int func_800655BC(unsigned char *, unsigned int, int, int);

void func_80063F40(unsigned char *actor)
{
    unsigned char *scratch = SCRATCHPAD;
    unsigned char mode;
    int dir;

    if (B(actor, 0x396) == 2) {
        return;
    }

    mode = B(D_8010C1ED, 0);
    if (mode == 0) {
        unsigned int goalAng;
        unsigned int bodyAng;
        unsigned int diff;
        int within;
        int absX;
        int absZ;
        unsigned int oppId;
        unsigned char *oppActor;
        int dx;
        int dz;
        unsigned char *table;

        if (B(D_8010C1D8, 0) != 0) {
            B(D_8010C1ED, 0) = 1;
            return;
        }

        goalAng = (unsigned int)B(actor, 0x399) << 7;
        bodyAng = B(actor, 0x3AA);
        diff = (bodyAng - goalAng) & 0xFF;
        if (diff < 0x81U) {
            within = ((signed int)diff < 0x18);
        } else {
            within = ((signed int)((goalAng - bodyAng) & 0xFF) < 0x18);
        }
        if (within) {
            func_800667E8(actor, 0);
            return;
        }

        absX = S(actor, 0x02);
        if (absX < 0) {
            absX = -absX;
        }
        if (absX >= 0x27E3) {
            absZ = S(actor, 0x06);
            if (absZ < 0) {
                absZ = -absZ;
            }
            if (absZ < 0x8B6) {
                func_800667E8(actor, 0);
                return;
            }
        }

        oppId = B(scratch, 0x31E + (B(actor, 0x399) ^ 1) * 2);
        oppActor = D_800FF748 + (oppId % 24) * 1000;
        dx = S(actor, 0x02) - S(oppActor, 0x02);
        dz = S(actor, 0x06) - S(oppActor, 0x06);
        if (dx * dx + dz * dz <= 0x3FFFF) {
            func_800667E8(actor, 0);
            return;
        }

        B(D_8010C1D8, 0x15) = B(D_8010C1D8, 0x15) + 1;
        func_80064B44(actor);
        table = (B(actor, 0x399) != 0) ? D_800C84B0 : D_800C84A8;
        dir = func_800653D4(table, 3, 3);
        if ((dir & 0xFF) == 0xFF) {
            dir = (int)B(actor, 0x399) << 7;
        }
        func_8005E09C(actor, dir & 0xFF, 0);
        return;
    }

    if (mode == 1) {
        unsigned char *table;
        unsigned char lockVal;
        unsigned int nearestId;
        unsigned char *nearestActor;
        unsigned int row;
        unsigned int stat;
        unsigned int goalAng;
        unsigned int bodyAng;
        unsigned int diff;
        int within;
        int absX;
        int absZ;
        int xSigned;
        unsigned char *ball;
        unsigned int ballAng;

        func_80064B44(actor);
        table = (B(actor, 0x399) != 0) ? D_800C84B0 : D_800C84A8;
        dir = func_800653D4(table, 3, 3);
        lockVal = B(D_8010C1D8, 0);
        if (lockVal == 2) {
            func_800667E8(actor, 0);
            return;
        }

        nearestId = func_8005CE30(B(actor, 0x38D)) & 0xFF;
        nearestActor = D_800FF748 + (nearestId % 24) * 1000;
        row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        stat = W(D_8010C328, row);
        if (func_8001E6B8((~stat) & 0xF) == 0) {
            int dx = S(actor, 0x02) - S(nearestActor, 0x02);
            int dz = S(actor, 0x06) - S(nearestActor, 0x06);
            int thresh = (int)((stat & 0xF) * ((unsigned int)B(D_800E6CB2, 0) << 14)) + 0x310000;
            if (dx * dx + dz * dz < thresh) {
                func_800667E8(actor, 0);
                return;
            }
        }

        goalAng = (unsigned int)B(actor, 0x399) << 7;
        bodyAng = B(actor, 0x3AA);
        diff = (bodyAng - goalAng) & 0xFF;
        if (diff < 0x81U) {
            within = ((signed int)diff < 0x18);
        } else {
            within = ((signed int)((goalAng - bodyAng) & 0xFF) < 0x18);
        }
        if (within) {
            func_800667E8(actor, 0);
            return;
        }

        /* Duplicate heading check present in original binary */
        goalAng = (unsigned int)B(actor, 0x399) << 7;
        bodyAng = B(actor, 0x3AA);
        diff = (bodyAng - goalAng) & 0xFF;
        if (diff < 0x81U) {
            within = ((signed int)diff < 0x18);
        } else {
            within = ((signed int)((goalAng - bodyAng) & 0xFF) < 0x18);
        }
        if (within) {
            func_800667E8(actor, 0);
            return;
        }

        absZ = S(actor, 0x06);
        if (absZ < 0) {
            absZ = -absZ;
        }
        if (absZ < 0x9B6) {
            xSigned = S(actor, 0x02);
            if (B(actor, 0x399) != 0) {
                xSigned = -xSigned;
            }
            if (xSigned >= 0x22E3) {
                func_800667E8(actor, 0);
                return;
            }
        }

        if (B(actor, 0x396) == 3) {
            goalAng = (unsigned int)B(actor, 0x399) << 7;
            bodyAng = B(actor, 0x3AA);
            diff = (bodyAng - goalAng) & 0xFF;
            if (diff < 0x81U) {
                within = ((signed int)diff < 0x22);
            } else {
                within = ((signed int)((goalAng - bodyAng) & 0xFF) < 0x22);
            }
            if (within) {
                absZ = S(actor, 0x06);
                if (absZ < 0) {
                    absZ = -absZ;
                }
                if (absZ < 0xCF4) {
                    xSigned = S(actor, 0x02);
                    if (B(actor, 0x399) != 0) {
                        xSigned = -xSigned;
                    }
                    if (xSigned >= 0x1CE3) {
                        unsigned int sA = W(D_8010C328, row) & 0xF;
                        unsigned int sB;
                        if (func_8001E6B8(100 - sA * 5) == 0) {
                            func_800667E8(actor, 0);
                            return;
                        }
                        sB = W(D_8010C32C, row) & 0xF;
                        if (func_8001E6B8(100 - sB * 5) == 0) {
                            func_800667E8(actor, 0);
                            return;
                        }
                    }
                }
            }
        }

        nearestId = func_8005CE30(B(actor, 0x38D)) & 0xFF;
        nearestActor = D_800FF748 + (nearestId % 24) * 1000;
        absX = S(nearestActor, 0x02);
        if (absX < 0) {
            absX = -absX;
        }
        absZ = S(nearestActor, 0x06);
        if (absZ < 0) {
            absZ = -absZ;
        }
        if (absX < 0x2E09 || absZ >= 0x8B6) {
            if (func_8001E6B8(5) == 0) {
                func_800667E8(actor, 1);
                return;
            }
        }

        if ((dir & 0xFF) == 0xFF) {
            dir = (int)B(actor, 0x399) << 7;
        }

        absX = S(actor, 0x02);
        if (absX < 0) {
            absX = -absX;
        }
        if (absX >= 0x2E09) {
            ball = (unsigned char *)W(scratch, 0x2F4);
            bodyAng = B(actor, 0x3AA);
            ballAng = B(ball, 0x3A8);
            diff = (bodyAng - ballAng) & 0xFF;
            if (diff < 0x81U) {
                within = ((signed int)diff < 0x31);
            } else {
                within = ((signed int)((ballAng - bodyAng) & 0xFF) < 0x31);
            }
            if (!within) {
                dir = (S(actor, 0x06) >= 0) ? 0xC0 : 0x40;
            }

            absX = S(actor, 0x02);
            if (absX < 0) {
                absX = -absX;
            }
            if (absX >= 0x2E09) {
                absZ = S(actor, 0x06);
                if (absZ < 0) {
                    absZ = -absZ;
                }
                if (absZ >= 0x8B7 && func_800652EC(actor, 0x300) != 0) {
                    unsigned int statE = H(D_8010C32E, row) & 0xF;
                    unsigned int statA;

                    if (func_8001E6B8(0x3C - (9 - statE) * 5) == 0) {
                        int passTarget;
                        ball = (unsigned char *)W(scratch, 0x2F4);
                        passTarget = func_800655BC(actor, B(ball, 0x3A8), 0, 0x30);
                        if ((passTarget & 0xFF) != 0xFF) {
                            B(D_8010C1D8, 0x14) = 2;
                            B(D_8010C1D8, 0x15) = 0x10;
                            B(D_8010C1D8, 0x02) = (unsigned char)passTarget;
                            return;
                        }
                    }

                    statA = W(D_8010C328, row) & 0xF;
                    if (func_8001E6B8(0x3C - statA * 5) != 0) {
                        return;
                    }
                    ball = (unsigned char *)W(scratch, 0x2F4);
                    bodyAng = B(actor, 0x3AA);
                    ballAng = B(ball, 0x3A8);
                    diff = (ballAng - bodyAng) & 0xFF;
                    if (diff < 0x81U) {
                        within = ((signed int)diff < 0x20);
                    } else {
                        within = ((signed int)((bodyAng - ballAng) & 0xFF) < 0x20);
                    }
                    if (within) {
                        func_800667E8(actor, 0);
                    }
                    return;
                }
            }
        }

        absX = S(actor, 0x02);
        if (absX < 0) {
            absX = -absX;
        }
        if (absX >= 0x2F21) {
            dir = (S(actor, 0x06) >= 0) ? 0xC0 : 0x40;
        }
        func_8005E09C(actor, dir & 0xFF, 0);
    }
}
