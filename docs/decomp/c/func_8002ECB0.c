/*
 * func_8002ECB0
 *
 * Purpose:
 *   Adjusts a desired pass/shot heading to avoid up to two predicted opponent
 *   blockers stored in Scratchpad+0x322..0x325. Calls func_80067860 to find
 *   nearest blockers along the candidate lane, then iteratively steps the
 *   heading (up to 16 attempts) until a clear lane is found.
 *
 * Arguments:
 *   actor          - pointer to 1000-byte player actor struct
 *   desiredHeading - initial byte angle (0..255 = 360 deg)
 *   radius         - lane search radius (squared and passed to func_80067860)
 *
 * Return value:
 *   Adjusted byte heading (0..255).
 *
 * Reads / Writes:
 *   Actor:
 *     +0x002 (s16) x position (R)
 *     +0x006 (s16) z position (R)
 *     +0x392 (u8)  team slot 0..10 (R)
 *     +0x398 (u8)  side 0/1 (R)
 *     +0x399 (u8)  attack direction flag 0/1 (R)
 *   Blocker Actor (D_800FF748[idx % 24]):
 *     +0x002 (s16) x position (R)
 *     +0x006 (s16) z position (R)
 *     +0x008 (s32) x velocity (R)
 *     +0x010 (s32) z velocity (R)
 *   Scratchpad (0x1F800000):
 *     +0x2F4 (u8*) pointer to ball actor (+0x02/+0x06 s16 x/z) (R)
 *     +0x322..+0x325 (u8[2][2]) blocker actor indices per attack direction,
 *       0xFF = empty (R)
 *   Globals:
 *     D_8010C328: player stat table (stride 12 per slot, 264 per side; low nibble = angle margin idx) (R)
 *     D_800C6C98: s16 table of extra angle margins indexed by stat nibble (R)
 *     D_800FF748: 24-entry array of 1000-byte actor structs (R)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x1991 = 6545 raw (~26.18 m) forward X threshold to enable extra angle margin
 *   -0x1FE1 = -8161 raw (~-32.64 m), 0x23E2 = 9186 raw (~36.74 m) X range for biased turn sweep
 *   Angle clearances by distance bucket (dist >> 8): 0..2 -> 18, 3..4 -> 16, 5 -> 14, >=6 -> 12
 *   Sweep steps: biased +/-3 (0xFD / 3) or alternating +step/-step with step += 3 over 16 tries
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_8010C328[];
extern unsigned char D_800C6C98[];
extern unsigned char D_800FF748[];

extern void func_80067860(unsigned int, int, int, unsigned int, unsigned int, int);
extern unsigned int func_800B1128(int);
extern unsigned int func_8005B36C(int, int, int, int);

unsigned int func_8002ECB0(unsigned char* actor, unsigned int desiredHeading, int radius) {
    int extraMargin;
    unsigned int origHeadingByte;
    unsigned int headingByte;
    unsigned int firstBlocker;
    unsigned int foldedHeading;
    int forwardX;
    int useBiasedStep;
    unsigned char biasedStep;
    int attempt;
    int altStep;
    unsigned int atkDir;
    unsigned char* blockerSlotBase;

    atkDir = B(actor, 0x399);
    if (atkDir != 0) {
        if (-S(actor, 0x2) < 0x1991) {
            extraMargin = 0;
        } else {
            unsigned int row = (unsigned int)B(actor, 0x392) * 12u + (unsigned int)B(actor, 0x398) * 264u;
            unsigned int stat = W(D_8010C328, row) & 0xFu;
            extraMargin = S(D_800C6C98, stat * 2u);
        }
    } else {
        if (S(actor, 0x2) < 0x1991) {
            extraMargin = 0;
        } else {
            unsigned int row = (unsigned int)B(actor, 0x392) * 12u + (unsigned int)B(actor, 0x398) * 264u;
            unsigned int stat = W(D_8010C328, row) & 0xFu;
            extraMargin = S(D_800C6C98, stat * 2u);
        }
    }

    origHeadingByte = desiredHeading & 0xFFu;
    headingByte = desiredHeading;
    func_80067860(
        B(actor, 0x399) ^ 1u,
        S(actor, 0x2),
        S(actor, 0x6),
        origHeadingByte,
        (unsigned int)(extraMargin + 0x10) & 0xFFu,
        radius * radius
    );

    blockerSlotBase = Scratchpad + ((B(actor, 0x399) ^ 1u) * 2u);
    firstBlocker = B(blockerSlotBase, 0x322);
    if (firstBlocker == 0xFFu) {
        return origHeadingByte;
    }

    /* Fold heading into [0..127] relative to 0 (note: asm negates desiredHeading in delay slot). */
    if (((0u - headingByte) & 0xFFu) < 0x81u) {
        foldedHeading = (0u - headingByte) & 0x7Fu;
    } else {
        foldedHeading = headingByte & 0x7Fu;
    }

    useBiasedStep = 0;
    biasedStep = 0;
    forwardX = (B(actor, 0x399) != 0) ? -S(actor, 0x2) : S(actor, 0x2);
    if (forwardX >= -0x1FE1 && forwardX < 0x23E2) {
        unsigned int dir = B(actor, 0x399);
        int inSector = 0;
        if (dir == 0u) {
            if (((foldedHeading - 0x20u) & 0xFFu) < 0x31u) {
                inSector = 1;
            }
        } else if (dir == 1u) {
            if (((foldedHeading - 0x30u) & 0xFFu) < 0x31u) {
                inSector = 1;
            }
        }
        if (inSector) {
            int rel = ((int)(B(actor, 0x399) << 7) - (int)(headingByte & 0xFFu)) + 0x100;
            unsigned int norm = (unsigned int)(rel % 256);
            useBiasedStep = 1;
            if ((norm & 0xFFu) < 0x81u) {
                biasedStep = 3;
            } else {
                biasedStep = 0xFD;
            }
        }
    }

    altStep = 3;
    for (attempt = 0; attempt < 16; attempt++) {
        int slot;
        for (slot = 0; slot < 2; slot++) {
            unsigned int idx = B(Scratchpad + ((B(actor, 0x399) ^ 1u) * 2u) + slot, 0x322);
            unsigned char* target = D_800FF748 + (idx % 24u) * 1000u;
            int dx = S(actor, 0x2) - S(target, 0x2);
            int dz = S(actor, 0x6) - S(target, 0x6);
            unsigned int leadFrames = func_800B1128(dx * dx + dz * dz) >> 8;
            int predX = (signed short)(H(target, 0x2) + (unsigned int)(SW(target, 0x8) * (int)leadFrames));
            int predZ = (signed short)(H(target, 0x6) + (unsigned int)(SW(target, 0x10) * (int)leadFrames));
            unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
            unsigned int blockerAngle = func_8005B36C(S(ball, 0x2), S(ball, 0x6), predX, predZ);
            unsigned int baseAngleMargin;
            unsigned int totalMargin;
            unsigned int diff;

            switch (leadFrames) {
            case 0:
            case 1:
            case 2:
                baseAngleMargin = 0x12;
                break;
            case 3:
            case 4:
                baseAngleMargin = 0x10;
                break;
            case 5:
                baseAngleMargin = 0xE;
                break;
            default:
                baseAngleMargin = 0xC;
                break;
            }

            totalMargin = baseAngleMargin + (unsigned int)extraMargin;
            diff = (blockerAngle - headingByte) & 0xFFu;
            if (diff < 0x81u) {
                if (diff < totalMargin) {
                    break;
                }
            } else {
                if (((headingByte - blockerAngle) & 0xFFu) < totalMargin) {
                    break;
                }
            }

            if (slot == 1) {
                return headingByte & 0xFFu;
            }
            if (B(Scratchpad + ((B(actor, 0x399) ^ 1u) * 2u), 0x323) == 0xFFu) {
                return headingByte & 0xFFu;
            }
        }

        if (useBiasedStep != 0) {
            headingByte += biasedStep;
        } else if ((attempt & 1) == 0) {
            headingByte += (unsigned int)altStep;
        } else {
            headingByte -= (unsigned int)altStep;
        }
        altStep += 3;
    }

    return headingByte & 0xFFu;
}
