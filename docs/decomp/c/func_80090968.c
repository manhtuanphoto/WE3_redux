/*
 * func_80090968
 *
 * Purpose:
 *   Checks whether an airborne/side-on ball arrival qualifies for an acrobatic
 *   volley/overhead/diving strike (states 0x8D, 0x8E, 0x8F, or 0x96) based on
 *   player jump/agility stat in D_8010C330, distance, ball height at Scratchpad+0x60,
 *   ball timer Scratchpad+0x31C, and the angular difference between ball heading
 *   and targetHeading. On success, sets the actor's heading, mirror foot, target
 *   snapshot, and state, returning 1 (else 0).
 *
 * Arguments:
 *   actor         (unsigned char*) - pointer to 1000-byte player actor struct
 *   dist          (unsigned int)   - squared or scaled distance to ball
 *   targetHeading (unsigned int)   - desired shot/clearance heading byte
 *   curHeading    (unsigned int)   - actor's current heading byte
 *
 * Return value:
 *   int - 1 if an acrobatic/volley state was triggered, 0 otherwise
 *
 * Reads / Writes:
 *   Actor fields:
 *     +0x392 (u8)  team slot (0..10) (R)
 *     +0x396 (u8)  state (W: 0x8D, 0x8E, 0x8F, or 0x96)
 *     +0x397 (u8)  substate (W = 0)
 *     +0x398 (u8)  side (0/1) (R)
 *     +0x3A4 (u8)  move heading (W)
 *     +0x3AB (u8)  facing/body offset heading (W = targetHeading +- 0x40)
 *     +0x3AC (u8)  mirror/foot flag (W)
 *     +0x3B9 (u8)  skill/stat byte (divided by 10 for timer threshold) (R)
 *     +0x3BC (s16) anchor/target X (0 = unset) (R)
 *     +0x3C2/+0x3C4/+0x3C6 (s16) target snapshot x/y/z (W)
 *     +0x3D0 (u8), +0x3D1 (u8) action timer/flags (W = 8, 0)
 *   Scratchpad (0x1F800000):
 *     +0x048 (u16), +0x060 (s16), +0x078 (u16) predicted ball x, y(height), z (R)
 *     +0x2F4 (u8*) pointer to ball actor (+0x3A0 speed, +0x3A4 heading) (R)
 *     +0x31C (s16) ball timer (frames since last touch) (R)
 *   Globals:
 *     D_8010C330 / D_8010C32F: player stat table (12 bytes/slot, 264 bytes/side) (R)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   Angles: 0x24 = 36 (~50.6 deg), 0x4D = 77 (~108.3 deg), 0x40 = 64 (90 deg)
 *   Ball speed 0x4000 = 16384 (64 raw/frame << 8 = 76.8 m/s)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_8010C32F[];
extern unsigned char D_8010C330[];

int func_80090968(unsigned char* actor, unsigned int dist, unsigned int targetHeading, unsigned int curHeading) {
    int tableOff = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
    int stat = (int)((W(D_8010C330, tableOff) >> 11) & 0x7F) + 100;
    int maxDist;
    int maxHeight;
    int bonus;
    unsigned char* ball;
    unsigned char ballHeading;
    unsigned int angleDiff;
    int stat13;
    int thresh;

    {
        int scaled = stat * 384;
        maxDist = ((scaled / 180) * scaled) / 180;
    }
    if ((unsigned int)maxDist < dist) {
        return 0;
    }

    bonus = B(D_8010C32F, tableOff) & 0xF;
    maxHeight = ((-736 * stat) / 180) + 6;
    if (S(Scratchpad, 0x060) < maxHeight - bonus) {
        return 0;
    }
    if (S(Scratchpad, 0x31C) < 4) {
        return 0;
    }

    if (S(actor, 0x3BC) != 0 && W(W(Scratchpad, 0x2F4), 0x3A0) >= 0x4000u) {
        int scaled = stat * 1280;
        maxDist = ((scaled / 180) * scaled) / 180;
        if ((unsigned int)maxDist < dist) {
            return 0;
        }
    } else {
        tableOff = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        stat = (int)((W(D_8010C330, tableOff) >> 11) & 0x7F) + 100;
        {
            int scaled = stat * 576;
            maxDist = ((scaled / 180) * scaled) / 180;
        }
        if ((unsigned int)maxDist < dist) {
            return 0;
        }
        maxHeight = (-656 * stat) / 180;
        if (S(Scratchpad, 0x060) < maxHeight) {
            return 0;
        }
        if (S(Scratchpad, 0x31C) < 16) {
            return 0;
        }
    }

    ball = (unsigned char*)W(Scratchpad, 0x2F4);
    ballHeading = B(ball, 0x3A4);
    angleDiff = (ballHeading - targetHeading) & 0xFF;
    if (angleDiff >= 0x81) {
        angleDiff = (targetHeading - ballHeading) & 0xFF;
    }
    if (angleDiff < 0x24 || angleDiff >= 0x4D) {
        return 0;
    }

    tableOff = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
    stat = (int)((W(D_8010C330, tableOff) >> 11) & 0x7F) + 100;
    stat13 = stat * 832;
    maxDist = ((stat13 / 180) * stat13) / 180;

    if ((unsigned int)maxDist < dist) {
        maxHeight = (-176 * stat) / 180;
        if (maxHeight < S(Scratchpad, 0x060)) {
            thresh = 17 - (int)(B(actor, 0x3B9) / 10u);
            if (S(Scratchpad, 0x31C) < thresh) {
                B(actor, 0x396) = 0x96;
                B(actor, 0x397) = 0;
                return 1;
            }
        } else {
            thresh = 21 - (int)(B(actor, 0x3B9) / 10u);
            if (S(Scratchpad, 0x31C) < thresh) {
                B(actor, 0x396) = 0x96;
                B(actor, 0x397) = 0;
                return 1;
            }
        }
    }

    B(actor, 0x3A4) = (unsigned char)targetHeading;
    B(actor, 0x3AC) = (unsigned char)(((targetHeading - curHeading) & 0xFF) < 0x81);
    H(actor, 0x3C2) = H(Scratchpad, 0x048);
    H(actor, 0x3C4) = H(Scratchpad, 0x060);
    H(actor, 0x3C6) = H(Scratchpad, 0x078);
    if (B(actor, 0x3AC) != 0) {
        B(actor, 0x3AB) = (unsigned char)((targetHeading & 0xFF) - 0x40);
    } else {
        B(actor, 0x3AB) = (unsigned char)((targetHeading & 0xFF) + 0x40);
    }
    B(actor, 0x3D0) = 8;
    B(actor, 0x3D1) = 0;

    stat = (int)((W(D_8010C330, B(actor, 0x392) * 12 + B(actor, 0x398) * 264) >> 11) & 0x7F) + 100;
    if (((-176 * stat) / 180) < S(Scratchpad, 0x060)) {
        B(actor, 0x396) = 0x8F;
    } else if (((-480 * stat) / 180) >= S(Scratchpad, 0x060)) {
        B(actor, 0x396) = 0x8E;
    } else {
        B(actor, 0x396) = 0x8D;
    }
    B(actor, 0x397) = 0;
    return 1;
}
