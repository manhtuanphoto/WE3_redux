/*
 * func_8002CA7C
 *
 * Purpose:
 *   Selects the best pass-target teammate actor ID for `actor` along `heading`.
 *   If +0x39A is non-zero (and actor is not a GK, id 1 or 12), updates +0x3C2
 *   via func_80054BA4 and sets `heading` toward that target's 3-frame predicted
 *   position. Scans all 11 teammates on the same side: skips ineligible actors
 *   (func_80052C6C != 0 unless state == 6), filters out teammates > 90 deg
 *   off-heading (>= 0x41) or > 8192 raw units (32.77 m) from the ball, nudges
 *   very close teammates (distSq <= 0x8FFFF, ~3.07 m) into state 0x1B/substate
 *   0x40 when +0x3C4 != 1, scores candidates by angular deviation and squared
 *   distance from the ball, and falls back to func_80067860 + func_80049D94
 *   if no teammate matched and +0x39A != 0.
 *
 * Arguments:
 *   actor:   (unsigned char*) passing player actor
 *   heading: (unsigned int) desired pass heading (0..255)
 *
 * Return value:
 *   (unsigned int) chosen teammate actor ID (+0x38D), or 0 if none found
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x2F4 (u8*) pointer to ball actor (R)
 *   actor / teammate (1000-byte structs in D_800FF748):
 *     +0x002 (s16) x position (R)
 *     +0x006 (s16) z position (R)
 *     +0x008 (s32) vx velocity (R)
 *     +0x010 (s32) vz velocity (R)
 *     +0x38D (u8)  actor id (R)
 *     +0x396 (u8)  state (R/W: set to 0x1B if close non-special teammate)
 *     +0x397 (u8)  substate (W: set to 0x40 with state 0x1B)
 *     +0x399 (u8)  attack direction flag (R)
 *     +0x39A (u8)  target-update / fallback mode flag (R)
 *     +0x3AA (u8)  body heading (R)
 *     +0x3C2 (u8/s16) current pass target index (R/W)
 *     +0x3C4 (s16) pass mode flag (1 = skip proximity clear-out) (R/W)
 *     +0x3D2 (u8)  penalty flag (adds +0x10 angle penalty) (R)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   3         = 3-frame (0.10 s) position prediction lookahead
 *   0x41      = 65 angle units (~91.4 deg) max cone half-width
 *   0x4000000 = 8192^2 raw^2 (32.768 m max distance from ball)
 *   0x8FFFF   = ~768^2 raw^2 (~3.07 m proximity threshold to push teammate to state 0x1B)
 *   0x30      = 48 (parameter to fallback func_80067860)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];

extern int func_80054BA4(unsigned char* actor, unsigned int curTarget);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_8005CE14(unsigned int actorId);
extern unsigned int func_80052C6C(unsigned char* actor);
extern void func_80067860(unsigned int atkDir, int x, int z, unsigned int bodyHeading, int a4, int a5);
extern unsigned int func_80049D94(unsigned char* actor);

unsigned int func_8002CA7C(unsigned char* actor, unsigned int heading) {
    unsigned char* bestActor = (unsigned char*)0;
    unsigned int bestScore = 0xFFFFFFFFU;
    unsigned int startIdx;
    unsigned char* teammate;
    int i;

    if (B(actor, 0x39A) != 0) {
        unsigned int id = B(actor, 0x38D);
        int updated;
        int slot;
        unsigned char* tgt;

        if (id == 1 || id == 12) {
            return B(actor, 0x3C2);
        }
        updated = func_80054BA4(actor, B(actor, 0x3C2));
        S(actor, 0x3C2) = (signed short)updated;
        slot = ((signed short)updated) % 24;
        tgt = D_800FF748 + slot * 0x3E8;
        bestActor = tgt;
        heading = func_8005B36C(
            S(actor, 0x2),
            S(actor, 0x6),
            (signed short)(H(tgt, 0x2) + SW(tgt, 0x8) * 3),
            (signed short)(H(tgt, 0x6) + SW(tgt, 0x10) * 3)
        );
    }

    startIdx = (func_8005CE14(B(actor, 0x38D)) & 0xFFU) % 24U;
    teammate = D_800FF748 + startIdx * 0x3E8;

    for (i = 0; i < 11; i++) {
        if (teammate != actor) {
            if ((func_80052C6C(teammate) & 0xFFU) == 0 || B(teammate, 0x396) == 6) {
                int px = (signed short)(H(teammate, 0x2) + SW(teammate, 0x8) * 3);
                int pz = (signed short)(H(teammate, 0x6) + SW(teammate, 0x10) * 3);
                unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
                int dx = S(ball, 0x2) - px;
                int dz = S(ball, 0x6) - pz;
                unsigned int distSq = (unsigned int)(dx * dx + dz * dz);
                unsigned int ang = func_8005B36C(S(actor, 0x2), S(actor, 0x6), px, pz);
                unsigned int diff = ang - heading;
                unsigned int absDiff;
                unsigned int absByte;

                if ((diff & 0xFFU) < 0x81U) {
                    absDiff = diff;
                } else {
                    absDiff = heading - ang;
                }
                absByte = absDiff & 0xFFU;

                if (S(actor, 0x3C4) != 1) {
                    if (absByte >= 0x41U || distSq > 0x4000000U) {
                        teammate += 0x3E8;
                        continue;
                    }
                    if (distSq <= 0x8FFFFU) {
                        B(teammate, 0x396) = 0x1B;
                        B(teammate, 0x397) = 0x40;
                        teammate += 0x3E8;
                        continue;
                    }
                } else {
                    if (absByte >= 0x41U || distSq > 0x4000000U) {
                        teammate += 0x3E8;
                        continue;
                    }
                }

                {
                    unsigned int score;
                    unsigned int penDiff = absDiff;
                    unsigned int penByte = absByte;

                    if (B(teammate, 0x3D2) != 0) {
                        penDiff += 0x10U;
                        penByte = penDiff & 0xFFU;
                    }
                    if (penByte < 0x11U) {
                        score = distSq;
                    } else if (penByte < 0x20U) {
                        score = distSq << 2;
                    } else {
                        score = ((penDiff & 0xF0U) + 0x10U) * distSq;
                    }

                    if (score <= bestScore) {
                        bestActor = teammate;
                        bestScore = score;
                    }
                }
            }
        }
        teammate += 0x3E8;
    }

    if (bestScore != 0xFFFFFFFFU) {
        return B(bestActor, 0x38D);
    }

    if (B(actor, 0x39A) == 0) {
        return 0;
    }

    func_80067860(B(actor, 0x399), S(actor, 0x2), S(actor, 0x6), B(actor, 0x3AA), 0x30, 0x1000000);
    bestActor = D_800FF748 + ((func_80049D94(actor) & 0xFFU) % 24U) * 0x3E8;
    if (B(actor, 0x38D) == B(bestActor, 0x38D)) {
        return 0;
    }
    S(actor, 0x3C4) = 0;
    return B(bestActor, 0x38D);
}
