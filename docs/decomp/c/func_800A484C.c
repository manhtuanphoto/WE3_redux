/*
 * func_800A484C
 *
 * Purpose:
 *   Assigns defending wall / set-piece marking slots (actor+0x3E3 = 0xFD,
 *   actor+0x3E4 = slot index 0..6) for free kicks and set pieces when match
 *   phase (Scratchpad+0x29A) is 2 or 4, and updates the assigned count in
 *   D_800E7000.
 *
 * Arguments:
 *   void
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x29A (u8)  match phase (2 or 4 handled; others return early) (R)
 *     +0x2AD (u8)  set-piece taker / primary actor ID (R)
 *     +0x2AE (u8)  secondary actor ID (R)
 *     +0x2F4 (u8*) pointer to ball actor (R)
 *     +0x2FF..+0x300 (u8[2]) per-side wall/tactic level (0, 1, 2 -> 3, 4, 5 defenders) (R)
 *   Actors (D_800FF748, stride 0x3E8 = 1000 bytes, 24 slots):
 *     +0x000 (u8)  active flag (R)
 *     +0x38D (u8)  actor ID (R)
 *     +0x391 (u8)  role/position code (0x19 = slot 0 priority; 0x11..0x17 = slot 1 candidate) (R)
 *     +0x392 (u8)  team slot 0..10 (R)
 *     +0x398 (u8)  side 0/1 (R)
 *     +0x399 (u8)  attack direction flag (R)
 *     +0x3E3 (u8)  set-piece marker flag (cleared to 0, set to 0xFD when assigned) (W)
 *     +0x3E4 (u8)  assigned set-piece marker slot index (0..6) (W)
 *   Ball (Scratchpad+0x2F4):
 *     +0x002 (s16) ball x position (R)
 *   Globals:
 *     D_800E7000 (u8)  number of assigned wall/marker players (W)
 *     D_8010C1D8 +0x3A (s16) cleared to 0 if phase 2 distance check fails (W)
 *     D_8010C212 (s16) set to 0xB4 (180) on entry (W)
 *     D_801002A5 (u8)  flag/actor passed to func_80067F78 in phase 4 (R)
 *     D_801002B1 (u8)  side/direction selector in phase 2 (R)
 *     D_8010C32F (u8[]) player stat table indexed by side*264 + slot*12 (low nibble = priority) (R)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0xB4   = 180 (6.0 s at 30 Hz) stored to D_8010C212
 *   0xBC1  = 3009 raw = 12.036 m (ball x threshold for attackDir == 0)
 *   -0xBC0 = -3008 raw = -12.032 m (ball x threshold for attackDir == 1)
 *   0x3E8  = 1000-byte actor struct stride; 0x7D0 = actor 2, 0x32C8 = actor 13
 *   0x55F0 = actor 22; 0xFD = 253 marker assignment flag
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800E7000[];
extern unsigned char D_800FF748[];
extern unsigned char D_801002A5[];
extern unsigned char D_801002B1[];
extern unsigned char D_8010C1D8[];
extern unsigned char D_8010C212[];
extern unsigned char D_8010C32F[];

extern int func_80067F78(unsigned int, unsigned int);

void func_800A484C(void) {
    unsigned char* actorsBase = D_800FF748;
    unsigned char* actorPtr;
    unsigned char* auxPtr = D_8010C1D8;
    unsigned char matchPhase;
    unsigned char sideLevel;
    unsigned char wallCount;
    unsigned char slotIds[2];
    unsigned int bestStat;
    unsigned int i;
    unsigned int extraSlot;

    S(D_8010C212, 0) = 0xB4; /* 180 = 6.0 s */
    B(D_800E7000, 0) = 0;

    matchPhase = B(Scratchpad, 0x29A);
    if (matchPhase == 2) {
        unsigned char* actor2AE = actorsBase + (unsigned int)B(Scratchpad, 0x2AE) * 0x3E8;
        unsigned char atkDir = B(actor2AE, 0x399);
        actorPtr = actorsBase + 0x7D0; /* actor 2 */

        if (atkDir == 0 && B(D_801002B1, 0) == 0) {
            goto check_ball_dir0;
        }
        if (atkDir != 1 || B(D_801002B1, 0) != 1) {
            actorPtr = actorsBase + 0x32C8; /* actor 13 */
        }
        if (B(actorPtr, 0x399) == 0) {
check_ball_dir0:
            if (S(W(Scratchpad, 0x2F4), 2) < 0xBC1) { /* < 12.036 m */
                goto assign_wall;
            }
        }
        if (B(actorPtr, 0x399) == 1 && S(W(Scratchpad, 0x2F4), 2) < -0xBC0) { /* < -12.032 m */
            goto assign_wall;
        }
        H(auxPtr, 0x3A) = 0;
        return;
    } else if (matchPhase == 4) {
        actorPtr = actorsBase + 0x7D0;
        if (func_80067F78(B(Scratchpad, 0x2AD), B(D_801002A5, 0)) != 0) {
            actorPtr = actorsBase + 0x32C8;
        }
    } else {
        return;
    }

assign_wall:
    sideLevel = B(Scratchpad, 0x2FF + B(actorPtr, 0x398));
    if (sideLevel == 1) {
        wallCount = 4;
    } else if (sideLevel < 2) {
        wallCount = 3;
    } else if (sideLevel == 2) {
        wallCount = 5;
    } else {
        wallCount = 3;
    }

    for (i = 0; (i & 0xFF) < 10; i++) {
        B(actorPtr, 0x3E3) = 0;
        actorPtr += 0x3E8;
    }

    matchPhase = B(Scratchpad, 0x29A);
    if (matchPhase == 2) {
        unsigned char* actor2AE = actorsBase + (unsigned int)B(Scratchpad, 0x2AE) * 0x3E8;
        unsigned char atkDir = B(actor2AE, 0x399);
        actorPtr = actorsBase + 0x7D0;
        if (atkDir == 0 && B(actorsBase, 0xB69) == 0) { /* D_800FF748+0xB69 == D_801002B1 */
            bestStat = 0;
        } else if (atkDir == 1 && B(actorsBase, 0xB69) == 1) {
            bestStat = 0;
        } else {
            actorPtr = actorsBase + 0x32C8;
            bestStat = 0;
        }
    } else if (matchPhase == 4) {
        actorPtr = actorsBase + 0x7D0;
        if (func_80067F78(B(Scratchpad, 0x2AD), B(actorsBase, 0xB5D)) != 0) { /* +0xB5D == D_801002A5 */
            actorPtr = actorsBase + 0x32C8;
        }
        bestStat = 0;
    } else {
        return;
    }

    slotIds[0] = 0;
    slotIds[1] = 0;
    for (i = 0; (i & 0xFF) < 10; i++) {
        if (B(actorPtr, 0) != 0) {
            unsigned char actorId = B(actorPtr, 0x38D);
            if (actorId != B(Scratchpad, 0x2AD) && actorId != B(Scratchpad, 0x2AE)) {
                unsigned char role = B(actorPtr, 0x391);
                if (role >= 0x11) {
                    if (role < 0x18) {
                        unsigned int row = (unsigned int)B(actorPtr, 0x392) * 12 +
                                           (unsigned int)B(actorPtr, 0x398) * 264;
                        unsigned int stat = B(D_8010C32F, row) & 0xF;
                        if (bestStat < stat) {
                            bestStat = stat;
                            slotIds[1] = actorId;
                        }
                    } else if (role == 0x19) {
                        slotIds[0] = actorId;
                    }
                }
            }
        }
        actorPtr += 0x3E8;
    }

    /* UNCLEAR: actorPtr points to the actor immediately following the 10-player block
     * (actor 12 or actor 23) after the loop above, and reads its side at +0x398. */
    sideLevel = B(Scratchpad, 0x2FF + B(actorPtr, 0x398));
    if (sideLevel == 1) {
        slotIds[1] = 0;
    } else if (sideLevel == 0) {
        slotIds[0] = 0;
        slotIds[1] = 0;
    }

    for (i = 0; (i & 0xFF) < 2; i++) {
        unsigned char id = slotIds[i & 0xFF];
        if (id != 0) {
            actorPtr = actorsBase + (id % 24) * 0x3E8;
            B(actorPtr, 0x3E3) = 0xFD;
            B(actorPtr, 0x3E4) = (unsigned char)i;
            wallCount--;
            B(D_800E7000, 0)++;
        }
    }

    if (B(Scratchpad, 0x29A) == 2) {
        return;
    }

    extraSlot = 2;
    actorPtr = actorsBase + 0x55F0; /* actor 22 */
    for (i = 0; (i & 0xFF) < 0x16; i++) {
        if (B(actorPtr, 0) != 0) {
            unsigned char actorId = B(actorPtr, 0x38D);
            if (actorId != B(Scratchpad, 0x2AD) && actorId != B(Scratchpad, 0x2AE)) {
                if (func_80067F78(B(Scratchpad, 0x2AD), actorId) == 0) {
                    wallCount--;
                    B(actorPtr, 0x3E3) = 0xFD;
                    B(actorPtr, 0x3E4) = (unsigned char)extraSlot;
                    B(D_800E7000, 0)++;
                    extraSlot++;
                }
                if ((wallCount & 0xFF) == 0 || (extraSlot & 0xFF) >= 7) {
                    return;
                }
            }
        }
        actorPtr -= 0x3E8;
    }
}
