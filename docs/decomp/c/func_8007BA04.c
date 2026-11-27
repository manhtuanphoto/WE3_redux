/*
 * func_8007BA04
 *
 * Purpose:
 *   Selects the set-piece taker (corner kick, free kick, or penalty/kickoff)
 *   among the 10 outfield players on the actor's team by checking designated
 *   taker slots via func_80054C54 or falling back to the best-rated candidate
 *   in D_8010C32C / D_8010C32E.
 *
 * Arguments:
 *   actor (u8 *): pointer to 1000-byte actor struct on the kicking team
 *
 * Return value:
 *   unsigned char: actor ID of the selected set-piece taker.
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x29A (u8)  match phase (4 = corner kick; 2 and others take free-kick/other paths)
 *   +0x2BE (s16) ball/event Z coordinate (determines left vs right corner taker)
 *   +0x2DD[2] (u8) per-side team ID
 *   +0x2E1 (u8)  mode (5 = special/training/demo mode)
 *   +0x2F4 (u32) pointer to ball actor
 *
 * Actor fields read:
 *   +0x000 (u8)  active flag
 *   +0x38D (u8)  actor ID
 *   +0x391 (u8)  position/role index (>= 6 eligible for close free kick)
 *   +0x392 (u8)  team slot (0..10)
 *   +0x398 (u8)  side (0/1)
 *   +0x399 (u8)  attack direction flag
 *
 * Global fields read:
 *   D_800FF748 (actor array, stride 0x3E8 = 1000 bytes; plus team/set-piece tables at
 *     +0x88A4, +0x8A13, +0x8A3E, +0x8A69, +0x8A94, +0x8ABF, +0xABDE, +0xABDF)
 *   D_8010C32C (u32[]) / D_8010C32E (u16[]) player stat tables (stride 12 per slot, 264 per side)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x3320 (13088) = 52.352 m (goal line X offset);
 *   0x3FFFFFF (dx^2 <= 8192^2) = 32.768 m range for close vs long free-kick taker.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010C32C[];
extern unsigned char D_8010C32E[];

extern unsigned int func_8005CE14(unsigned int actorId);
extern unsigned int func_80054C54(unsigned char *actor, unsigned int slot);

unsigned char func_8007BA04(unsigned char *actor)
{
    unsigned int gkId = func_8005CE14(B(actor, 0x38D)) & 0xFF;
    unsigned char *curActor = D_800FF748 + gkId * 0x3E8 + 0x3E8;
    unsigned char *bestActor = curActor;
    int bestScore = 0xFF;
    int i;

    for (i = 0; i < 0xA; i++) {
        if (B(curActor, 0) != 0) {
            unsigned char phase = B(Scratchpad, 0x29A);
            unsigned int row;
            int score;
            unsigned int res;

            if (phase == 4) {
                unsigned int cornerSide;
                unsigned char slot;

                if (B(Scratchpad, 0x2E1) == 5 && B(D_800FF748, 0xABDE) == 2) {
                    cornerSide = B(D_800FF748, 0xABDF) >> 1;
                } else {
                    signed short bz = S(Scratchpad, 0x2BE);
                    if (B(curActor, 0x399) != 0) {
                        cornerSide = (bz > 0) ? 0 : 1;
                    } else {
                        cornerSide = (bz >= 0) ? 1 : 0;
                    }
                }

                if (cornerSide == 0) {
                    slot = B(D_800FF748, B(Scratchpad, 0x2DD + B(curActor, 0x398)) + 0x8A3E);
                } else {
                    slot = B(D_800FF748, B(Scratchpad, 0x2DD + B(curActor, 0x398)) + 0x8A13);
                }

                res = func_80054C54(curActor, slot);
                if (res != 0) {
                    return (unsigned char)res;
                }

                row = B(curActor, 0x392) * 12 + B(curActor, 0x398) * 264;
                score = 9 - (H(D_8010C32E, row) & 0xF);
                if (score < bestScore || bestScore == 0xFF) {
                    bestScore = score;
                    bestActor = curActor;
                }
            } else if (B(D_800FF748, 0x88A4) == 3) {
                unsigned char slot = B(D_800FF748, B(Scratchpad, 0x2DD + B(curActor, 0x398)) + 0x8A69);

                res = func_80054C54(curActor, slot);
                if (res != 0) {
                    return (unsigned char)res;
                }

                row = B(curActor, 0x392) * 12 + B(curActor, 0x398) * 264;
                score = 9 - ((W(D_8010C32C, row) >> 4) & 0xF);
                if (score < bestScore || bestScore == 0xFF) {
                    bestScore = score;
                    bestActor = curActor;
                }
            } else {
                unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
                int dx = S(ball, 2) + ((B(curActor, 0x399) != 0) ? 0x3320 : -0x3320);

                if (dx < 0) {
                    dx = -dx;
                }
                if (dx * dx <= 0x3FFFFFF) {
                    if (B(curActor, 0x391) >= 6) {
                        unsigned char teamIdx = B(Scratchpad, 0x2DD + B(curActor, 0x398));
                        unsigned char slot = B(D_800FF748, teamIdx + 0x8ABF);

                        if (slot == 0xFF && (teamIdx == 0x22 || teamIdx == 0x28)) {
                            slot = 0;
                        }
                        res = func_80054C54(curActor, slot);
                        if (res != 0) {
                            return (unsigned char)res;
                        }

                        row = B(curActor, 0x392) * 12 + B(curActor, 0x398) * 264;
                        score = 9 - ((W(D_8010C32C, row) >> 4) & 0xF);
                        if (score < bestScore || bestScore == 0xFF) {
                            bestScore = score;
                            bestActor = curActor;
                        }
                    }
                } else {
                    unsigned char slot = B(D_800FF748, B(Scratchpad, 0x2DD + B(curActor, 0x398)) + 0x8A94);

                    res = func_80054C54(curActor, slot);
                    if (res != 0) {
                        return (unsigned char)res;
                    }

                    row = B(curActor, 0x392) * 12 + B(curActor, 0x398) * 264;
                    score = W(D_8010C32C, row) & 0xF;
                    if (bestScore < score || bestScore == 0xFF) {
                        bestScore = score;
                        bestActor = curActor;
                    }
                }
            }
        }

        curActor += 0x3E8;
    }

    return B(bestActor, 0x38D);
}
