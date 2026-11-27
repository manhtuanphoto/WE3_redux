/*
 * func_80046F8C
 * Purpose: Initiates and updates a dribbling player's turn / cut / dribble-knock-on
 *          transition based on pad direction (0x1F80028C -> D_800C6B74), snapping
 *          headings to 45-degree increments or entering turn/cut/sprint-touch animations.
 * Arguments:
 *   - actor (unsigned char *): pointer to 1000-byte player actor struct
 * Return value: void
 * Actor fields read/written:
 *   - +0x38D (u8): actor ID
 *   - +0x38E (u16): current animation ID (checked for 9, 0x1B, 0x26, 0x34)
 *   - +0x390 (u8): current animation frame
 *   - +0x392 (u8): team slot (0..10)
 *   - +0x396 (u8): actor state (0 = idle, 1 = move, 2 = dribble/turn, 3 = knock-on/sprint)
 *   - +0x397 (u8): substate (0, 0x10, 0x30, 0x50, 0x60, 0x70, 5)
 *   - +0x398 (u8): team side (0/1)
 *   - +0x399 (u8): attack direction flag (0/1)
 *   - +0x39A (u8): AI/assisted flag
 *   - +0x39B (u8): jitter/variance flag (adds rand(6) to turn divisor)
 *   - +0x3A0 (u32): movement speed (zeroed on sharp reversal, clamped to +0x3B3 or 0x22)
 *   - +0x3A4 (u8): move heading
 *   - +0x3AA (u8): body heading
 *   - +0x3AB (u8): target heading (from D_800C6B74[padDir])
 *   - +0x3AC (u8): mirror/foot flag
 *   - +0x3B0 (u8): turn frame duration
 *   - +0x3B3 (u8): dribble speed cap
 *   - +0x3B4 (u8): max sprint speed
 *   - +0x3C6 (u16): set to 0xFF
 *   - +0x3C8 (s16/u16): turn step rate (32 / turnDuration, or (diff / 9) + 1)
 *   - +0x3CA (u16): cleared to 0
 *   - +0x3CC (u16): turn variant/angle modifier (0, 0xA, or 0xB)
 *   - +0x3D1 (u8): cleared to 0 on knock-on kick
 * Scratchpad reads/writes:
 *   - 0x1F80028C (u8): pad direction index
 *   - 0x1F8002A9 (u8): ball owner ID (set to actor ID on knock-on)
 *   - 0x1F80031C (s16): ball timer (set to (attackDir ^ 1) - 4 on knock-on)
 *   - 0x1F800332 (u8): dribble touch counter (incremented from 0 to 1)
 * Globals read:
 *   - D_800C6B74[]: pad direction -> angle byte lookup table
 *   - D_800C6D8C[]: agility stat -> base turn frames lookup table
 *   - D_8010C32A / D_8010C330: player stat words indexed by side*264 + slot*12
 * Magic numbers:
 *   - 0x10 / 0xE0: 45-degree (32/256) snap rounding (+16 then & ~31)
 *   - 0x21 (46.4 deg), 0x40 (90 deg), 0x41 (91.4 deg), 0x50 (112.5 deg),
 *     0x60 (135 deg), 0x71 (158.9 deg): angular turn thresholds
 *   - 0x22 (34 raw = 0.136 m/frame): speed cap during 90-deg cut
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCHPAD ((unsigned char *)0x1F800000)

extern unsigned char D_800C6B74[];
extern unsigned char D_800C6D8C[];
extern unsigned char D_8010C32A[];
extern unsigned char D_8010C330[];

extern int func_80054F1C(unsigned char *);
extern unsigned int func_80049398(unsigned char *, unsigned int, unsigned int);
extern unsigned int func_8005B7CC(unsigned int, unsigned int, unsigned int);
extern void func_8005BA30(unsigned char *, unsigned int, unsigned int);
extern void func_8004B0F0(unsigned char *, unsigned int, unsigned int, unsigned int);
extern int func_8001E6B8(int);

void func_80046F8C(unsigned char *actor)
{
    unsigned char *scratch = SCRATCHPAD;
    unsigned char padDir;
    unsigned int angDiff = 0;
    unsigned int turnSide;
    unsigned int diffByte;

    if (B(scratch, 0x332) == 0) {
        B(scratch, 0x332) = 1;
    }

    padDir = B(scratch, 0x28C);
    B(actor, 0x3AB) = B(D_800C6B74, padDir);

    if (B(actor, 0x39A) != 0 && func_80054F1C(actor) != 0) {
        unsigned int targetAng = B(actor, 0x3AB);
        unsigned int altAng = func_80049398(actor, targetAng, 0x90000U);
        unsigned int diff = (targetAng - altAng) & 0xFF;
        int closeToTarget;

        angDiff = altAng;
        if (diff < 0x81U) {
            closeToTarget = ((signed int)diff < 0x40);
        } else {
            closeToTarget = ((signed int)((altAng - targetAng) & 0xFF) < 0x40);
        }

        if (!closeToTarget) {
            unsigned int bodyAng = B(actor, 0x3AA);
            unsigned int bodyDiff = (bodyAng - altAng) & 0xFF;
            int closeToBody;

            if (bodyDiff < 0x81U) {
                closeToBody = ((signed int)bodyDiff < 0x31);
            } else {
                closeToBody = ((signed int)((altAng - bodyAng) & 0xFF) < 0x31);
            }

            if (closeToBody && B(actor, 0x396) != 0) {
                B(actor, 0x396) = 0;
                B(actor, 0x397) = 0;
                return;
            }

            {
                unsigned int turned = func_8005B7CC(altAng & 0xFF, B(actor, 0x3AB), 0x40);
                unsigned char snapped = (unsigned char)((turned + 0x10) & 0xFFFFFFE0U);
                unsigned int goalAng = (unsigned int)B(actor, 0x399) << 7;
                unsigned int goalDiff = (snapped - goalAng) & 0xFF;
                int towardGoal;

                B(actor, 0x3AB) = snapped;
                if (goalDiff < 0x81U) {
                    towardGoal = ((signed int)goalDiff < 0x60);
                } else {
                    towardGoal = ((signed int)((goalAng - snapped) & 0xFF) < 0x60);
                }
                if (!towardGoal) {
                    W(actor, 0x3A0) = 0;
                }
            }
        }
    }

    if (B(actor, 0x396) != 2) {
        unsigned char snappedBody = (unsigned char)((B(actor, 0x3AA) + 0x10) & 0x1E0);
        unsigned char targetAng = B(actor, 0x3AB);
        unsigned char curBody;

        B(actor, 0x3AA) = snappedBody;
        curBody = B(actor, 0x3AA);
        B(actor, 0x3A4) = snappedBody;

        if (((curBody - targetAng) & 0xFF) < 0x81U) {
            angDiff = curBody - targetAng;
        } else {
            angDiff = targetAng - curBody;
        }

        if ((angDiff & 0xFF) < 0x10U) {
            if (B(actor, 0x39A) == 0) {
                unsigned char t = B(actor, 0x3AB);
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 0;
                B(actor, 0x3A4) = t;
                B(actor, 0x3AA) = t;
                return;
            }
            if (B(actor, 0x396) != 0) {
                B(actor, 0x396) = 0;
                B(actor, 0x397) = 0;
            }
            return;
        }
    }

    B(actor, 0x396) = 2;
    H(actor, 0x3CA) = 0;
    turnSide = (((B(actor, 0x3AB) - B(actor, 0x3AA)) & 0xFF) < 0x81U);
    H(actor, 0x3C6) = 0xFF;
    diffByte = angDiff & 0xFF;

    if (diffByte >= 0x21U && W(actor, 0x3A0) == 0) {
        unsigned int targetAng = B(actor, 0x3AB);
        unsigned int checkAng = func_80049398(actor, targetAng, 0x10000U);
        int ok;

        if (((checkAng - targetAng) & 0xFF) < 0x81U) {
            unsigned int a = func_80049398(actor, B(actor, 0x3AB), 0x10000U);
            ok = ((signed int)((a - B(actor, 0x3AB)) & 0xFF) < 0x50);
        } else {
            unsigned int a = func_80049398(actor, B(actor, 0x3AB), 0x10000U);
            ok = ((signed int)((B(actor, 0x3AB) - a) & 0xFF) < 0x50);
        }

        if (!ok) {
            B(actor, 0x396) = 0;
            B(actor, 0x397) = 0;
            return;
        }

        {
            unsigned int row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
            unsigned int footPref = (W(D_8010C330, row) >> 4) & 1;

            if ((turnSide & 0xFF) == footPref) {
                func_8005BA30(actor, 8, 0);
                B(actor, 0x397) = 0x70;
                B(actor, 0x3AC) = (unsigned char)turnSide;
            } else {
                func_8005BA30(actor, 6, 0);
                B(actor, 0x3AC) = (unsigned char)(turnSide ^ 1);
                B(actor, 0x397) = 0x60;
            }

            W(actor, 0x3A0) = 0;
            H(actor, 0x3C8) = (unsigned short)((diffByte / 9) + 1);
            return;
        }
    }

    if (diffByte >= 0x71U) {
        unsigned int targetAng = B(actor, 0x3AB);
        unsigned int checkAng = func_80049398(actor, targetAng, 0x10000U);
        int ok;

        if (((checkAng - targetAng) & 0xFF) < 0x81U) {
            unsigned int a = func_80049398(actor, B(actor, 0x3AB), 0x10000U);
            ok = ((signed int)((a - B(actor, 0x3AB)) & 0xFF) < 0x50);
        } else {
            unsigned int a = func_80049398(actor, B(actor, 0x3AB), 0x10000U);
            ok = ((signed int)((B(actor, 0x3AB) - a) & 0xFF) < 0x50);
        }

        if (!ok) {
            B(actor, 0x396) = 0;
            B(actor, 0x397) = 0;
            return;
        }

        B(actor, 0x3AA) = (unsigned char)((B(actor, 0x3AA) + 0x10) & 0x1E0);
        if (W(actor, 0x3A0) >= (unsigned int)(B(actor, 0x3B4) - 4) && B(scratch, 0x332) >= 2) {
            func_8005BA30(actor, 7, 0);
            func_8004B0F0(actor, B(actor, 0x3AA), 8, 0);
            B(scratch, 0x2A9) = B(actor, 0x38D);
            B(actor, 0x3D1) = 0;
            S(scratch, 0x31C) = (signed short)((B(actor, 0x399) ^ 1) - 4);
            B(actor, 0x396) = 3;
            B(actor, 0x397) = 5;
            return;
        } else {
            unsigned int row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
            unsigned int footPref = (W(D_8010C330, row) >> 4) & 1;

            func_8005BA30(actor, 0x0C, 0);
            B(actor, 0x3AC) = (unsigned char)footPref;
            if (footPref != 0) {
                B(actor, 0x3AA) = B(actor, 0x3AA) - 1;
            } else {
                B(actor, 0x3AA) = B(actor, 0x3AA) + 1;
            }
            B(actor, 0x397) = 0x50;
            return;
        }
    }

    if (diffByte >= 0x41U) {
        unsigned int row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        unsigned int statA;

        func_8005BA30(actor, 0x0A, 0);
        B(actor, 0x3AC) = (unsigned char)turnSide;
        statA = H(D_8010C32A, row) & 0xF;
        if (statA == 8) {
            H(actor, 0x3CC) = 0x0B;
        } else if (statA == 9) {
            H(actor, 0x3CC) = 0x0A;
        } else {
            H(actor, 0x3CC) = 0;
        }

        W(actor, 0x3A0) = W(actor, 0x3A0) + 5;
        if (W(actor, 0x3A0) >= 0x23U) {
            W(actor, 0x3A0) = 0x22;
        }
        B(actor, 0x397) = 0x30;
        return;
    }

    if (diffByte == 0x40U) {
        unsigned int row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        unsigned int statA = H(D_8010C32A, row) & 0xF;

        if (statA == 8) {
            func_8005BA30(actor, 0x0A, 0);
            H(actor, 0x3CC) = 0x0B;
        } else if (statA == 9) {
            func_8005BA30(actor, 0x0A, 1);
            H(actor, 0x3CC) = 0x0B;
        } else {
            func_8005BA30(actor, 0x0A, 0);
            H(actor, 0x3CC) = 0;
        }

        W(actor, 0x3A0) = W(actor, 0x3A0) + 5;
        if (W(actor, 0x3A0) >= 0x23U) {
            W(actor, 0x3A0) = 0x22;
        }
        B(actor, 0x397) = 0x10;
        B(actor, 0x3AC) = (unsigned char)turnSide;
        return;
    }

    {
        unsigned short animId = H(actor, 0x38E);
        unsigned int row;
        unsigned int turnFrames;
        int turnStep;
        unsigned int spdCap;

        if (animId == 0x26 || animId == 0x1B || animId == 0x34) {
            unsigned int frame;
            if (B(actor, 0x3AC) == 1) {
                frame = B(actor, 0x390);
            } else {
                frame = ((unsigned int)B(actor, 0x390) + 8) % 16;
            }
            func_8005BA30(actor, 9, frame);
        } else if (animId != 9) {
            func_8005BA30(actor, 9, 0);
        }

        row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        turnFrames = B(D_800C6D8C, H(D_8010C32A, row) & 0xF);
        if (B(actor, 0x39B) != 0) {
            turnFrames = turnFrames + func_8001E6B8(6);
        }

        turnStep = 32 / (signed int)(turnFrames & 0xFF);
        H(actor, 0x3C8) = (unsigned short)turnStep;
        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), turnStep);
        B(actor, 0x3A4) = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3A4), (S(actor, 0x3C8) << 1) & 0xFE);

        spdCap = B(actor, 0x3B3);
        W(actor, 0x3A0) = W(actor, 0x3A0) + ((spdCap / 6) & 0xFF);
        if (W(actor, 0x3A0) > B(actor, 0x3B3)) {
            W(actor, 0x3A0) = B(actor, 0x3B3);
        }

        B(actor, 0x3B0) = (unsigned char)turnFrames;
        B(actor, 0x397) = 0;
        B(actor, 0x3AC) = (unsigned char)turnSide;
    }
}
