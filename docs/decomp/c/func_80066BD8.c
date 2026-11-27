/*
 * func_80066BD8
 *
 * Purpose:
 *   Scans a team's 10 outfield actors (starting from the actor at actorId % 24)
 *   to select the primary (scratch 0x31E[atk*2]) and secondary (scratch 0x31F[atk*2])
 *   ball-chasing/receiving players, as well as close loose-ball interceptors
 *   (scratch 0x3D1[atk]), across three ball states: ball owned, ball in flight
 *   (0x2B0 == 1), and free ball. Also invokes func_80067860 and updates support-bit
 *   0x3D0[atk] when the team holds possession.
 *
 * Arguments:
 *   actorId - starting actor index (masked to u8, modulo 24; typically slot 1 or 12)
 *
 * Return value:
 *   void
 *
 * Scratchpad (0x1F800000) reads/writes:
 *   - 0x36/0x66 (s16, R): ball current/near-term predicted X/Z
 *   - 0x40/0x70 (s16, R): ball further predicted X/Z
 *   - 0xFE/0x100 (s16, R): alternate pass/shot predicted X/Z (used when ball +0x3CC >= 2)
 *   - 0x240/0x241 (u8, R): special actor ids (e.g., goalkeepers / designated players)
 *   - 0x2A9 (u8, R): ball owner actor id (0 = free)
 *   - 0x2AA (u8, R): previous ball owner actor id
 *   - 0x2B0 (u32, R): ball event code (1 = pass/shot in flight)
 *   - 0x2B4/0x2B6 (s16, R): pass/kick target X/Z (0x7FFF = unset)
 *   - 0x2CF (u8[2], R): team possession flags
 *   - 0x2F4 (u32, R): pointer to ball actor
 *   - 0x30B (u8, R): tactical/role selection mode (0..5+, dispatched via switch)
 *   - 0x31E/0x31F (u8[2], R/W): primary/secondary actor ids indexed by attackDir * 2
 *   - 0x322 (u8[2], R): designated support/target actor id indexed by attackDir * 2
 *   - 0x3D0 (u8, R/W): bitmask per attackDir (bit atk set/cleared based on possession & 0x322)
 *   - 0x3D1 (u8[2], W): close loose-ball actor id per attackDir (0xFF if none)
 *
 * Actor fields read:
 *   - +0x00 (u8): active flag
 *   - +0x02 (s16): X position
 *   - +0x04 (s16): Y position (ball height checked >= -0x1FF)
 *   - +0x06 (s16): Z position
 *   - +0x38D (u8): actor id
 *   - +0x391 (u8): tactical role / formation slot (1..25)
 *   - +0x398 (u8): side (0/1)
 *   - +0x399 (u8): attack direction flag (0/1)
 *   - +0x39A (u8): state/type flag (checked != 1 for 0x240/0x241 dist scaling)
 *   - +0x3A8 (u8): ball heading (passed to func_80067860 & lookahead)
 *   - +0x3CC (s16): ball flight/bounce counter (checked >= 2)
 *   - +0x3E1 (u8): marked opponent actor id
 *
 * Globals read:
 *   - D_800FF748: 24 * 1000-byte actor pool
 *   - D_8010C1D8 (u8): override flag skipping tactical role penalties when nonzero
 *
 * Magic numbers / units (250 raw = 1 m, 30 frames/s):
 *   - 0x340 (832) = 3.328 m (lookahead along ball heading)
 *   - 0x100 (256) = 1.024 m (ahead-of-ball X margin)
 *   - -0x1109 (-4361) = -17.444 m (ball X threshold separating 0x640000 vs 0xC40000 penalty)
 *   - 0x640000 (sqrt = 2560 = 10.24 m), 0xC40000 (sqrt = 3584 = 14.336 m),
 *     0x240000 (sqrt = 1536 = 6.144 m), 0x100000 (sqrt = 1024 = 4.096 m),
 *     0x90000 (sqrt = 768 = 3.072 m): squared-distance cost penalties
 *   - 0x1FE2 (8162) = 32.648 m (ball abs(X) threshold in mode 4/5/default)
 *   - -0x1FF (-511) = -2.044 m (ball height threshold for 0x3D1)
 *   - 0x23FFF (sqrt ~ 384 = 1.536 m): loose-ball proximity threshold for 0x3D1
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010C1D8;

extern void func_80067860(int, int, int, int, int, int);
extern int func_8005CE14(int);
extern int func_8001E694(int, int);
extern int func_8001E5A4(int, int);

void func_80066BD8(unsigned int actorId)
{
    unsigned char *actor;
    unsigned char atk;
    int hasSupport = 0;
    unsigned int bestPrimaryCost;
    unsigned int bestPrimarySecCost;
    unsigned int bestSecondCost;
    unsigned char *slotOut;
    int i;

    actor = D_800FF748 + ((actorId & 0xFF) % 24) * 1000;
    atk = B(actor, 0x399);

    if (B(Scratchpad, 0x2CF + B(actor, 0x398)) == 1) {
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        unsigned char idx;

        func_80067860(atk, S(Scratchpad, 0x40), S(Scratchpad, 0x70), B(ball, 0x3A8), 0x10, 0xA35C400);
        idx = B(Scratchpad, 0x322 + B(actor, 0x399) * 2);
        if (idx == 0xFF || (func_8005CE14(B(actor, 0x38D)) & 0xFF) == idx) {
            hasSupport = 1;
            B(Scratchpad, 0x3D0) |= (unsigned char)(1 << atk);
        } else {
            /* UNCLEAR: in original asm, $s7 (hasSupport) is uninitialized on this path */
            B(Scratchpad, 0x3D0) &= (unsigned char)~(1 << atk);
        }
    } else {
        hasSupport = 0;
        B(Scratchpad, 0x3D0) &= (unsigned char)~(1 << atk);
    }

    bestPrimarySecCost = 0xFFFFFFFFU;
    bestSecondCost = 0xFFFFFFFFU;
    B(Scratchpad, 0x3D1 + atk) = 0xFF;

    slotOut = Scratchpad + atk * 2;

    if (B(Scratchpad, 0x2A9) != 0) {
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        unsigned char ballHdg = B(ball, 0x3A8);
        signed short lookX = (signed short)(S(Scratchpad, 0x36) + func_8001E694(ballHdg, 0x340));
        signed short lookZ = (signed short)(S(Scratchpad, 0x66) + func_8001E5A4(ballHdg, 0x340));

        bestPrimaryCost = 0xFFFFFFFFU;

        for (i = 0; i < 10; i++, actor += 1000) {
            unsigned char id;
            unsigned char owner;
            unsigned int cost1;
            unsigned int cost2;

            if (B(actor, 0x00) == 0) {
                continue;
            }

            owner = B(Scratchpad, 0x2A9);
            id = B(actor, 0x38D);
            if (id == owner) {
                cost1 = 0;
                cost2 = 0;
            } else if (B(actor, 0x39A) != 1 && (id == B(Scratchpad, 0x240) || id == B(Scratchpad, 0x241))) {
                int dx = S(actor, 0x02) - S(Scratchpad, 0x36);
                int dz = S(actor, 0x06) - S(Scratchpad, 0x66);
                int d2 = dx * dx + dz * dz;
                cost1 = (unsigned int)(d2 / 65536);
                cost2 = cost1;
            } else {
                signed short px = S(actor, 0x02);
                signed short pz = S(actor, 0x06);
                int dx0 = px - S(Scratchpad, 0x36);
                int dz0 = pz - S(Scratchpad, 0x66);

                cost1 = (unsigned int)(dx0 * dx0 + dz0 * dz0);
                if (B(Scratchpad, 0x2CF + B(actor, 0x398)) == 1) {
                    int dx1 = px - lookX;
                    int dz1 = pz - lookZ;
                    signed short bx = S((unsigned char *)W(Scratchpad, 0x2F4), 0x02);
                    unsigned char pAtk = B(actor, 0x399);
                    int limitX = (pAtk == 0) ? (bx + 0x100) : (-bx + 0x100);
                    int ahead = (pAtk != 0) ? (limitX < -px) : (limitX < px);

                    cost2 = (unsigned int)(dx1 * dx1 + dz1 * dz1);
                    if (ahead) {
                        signed short refX = S(Scratchpad, 0x36);
                        if (pAtk != 0) {
                            refX = -refX;
                        }
                        cost1 += (refX < -0x1109) ? 0x640000U : 0xC40000U;
                        cost2 += 0x100000U;
                    } else if (id == B(Scratchpad, 0x31E + pAtk * 2)) {
                        cost1 >>= 1;
                    }
                } else {
                    cost2 = cost1;
                }

                if (D_8010C1D8 == 0) {
                    unsigned char role = B(actor, 0x391);
                    unsigned char oppMark = B(Scratchpad, 0x31E + (B(actor, 0x399) ^ 1) * 2);
                    unsigned char mark = B(actor, 0x3E1);

                    switch (B(Scratchpad, 0x30B)) {
                    case 0:
                    case 1:
                        switch (role) {
                        case 1: case 2: case 3: case 4: case 5:
                            cost2 = 0xFFFFFFFFU;
                            break;
                        case 17: case 18:
                        case 21: case 22: case 23: case 24: case 25:
                            cost1 <<= 1;
                            if (mark == 0) {
                                cost2 = 0xFFFFFFFFU;
                            } else if (mark != oppMark) {
                                cost1 = 0xFFFFFFFFU;
                                cost2 = 0xFFFFFFFFU;
                            }
                            break;
                        default:
                            break;
                        }
                        break;

                    case 2:
                        switch (role) {
                        case 1: case 2: case 3: case 4: case 5:
                            cost2 = 0xFFFFFFFFU;
                            break;
                        case 17: case 18: case 19: case 20:
                        case 21: case 22: case 23: case 24: case 25:
                            if (!hasSupport) {
                                cost1 += 0x240000U;
                            }
                            cost2 = 0xFFFFFFFFU;
                            break;
                        default:
                            break;
                        }
                        break;

                    case 3:
                        switch (role) {
                        case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8:
                            cost2 = 0xFFFFFFFFU;
                            break;
                        case 17: case 18: case 19: case 20:
                        case 21: case 22: case 23: case 24: case 25:
                            if (!hasSupport) {
                                if (mark == 0 || mark != oppMark) {
                                    cost2 += 0x100000U;
                                }
                            }
                            break;
                        default:
                            break;
                        }
                        break;

                    default:
                        if (role >= 17 && role < 26) {
                            if (!hasSupport) {
                                signed short bx = S((unsigned char *)W(Scratchpad, 0x2F4), 0x02);
                                if (bx < 0) {
                                    bx = -bx;
                                }
                                if (bx < 0x1FE2 && mark != 0 && mark != oppMark) {
                                    cost2 += 0x90000U;
                                }
                            }
                        }
                        break;
                    }
                }
            }

            if (cost1 < bestPrimaryCost) {
                if (bestPrimarySecCost < bestSecondCost) {
                    bestSecondCost = bestPrimarySecCost;
                    B(slotOut, 0x31F) = B(slotOut, 0x31E);
                }
                bestPrimaryCost = cost1;
                bestPrimarySecCost = cost2;
                B(slotOut, 0x31E) = B(actor, 0x38D);
            } else if (cost2 < bestSecondCost) {
                bestSecondCost = cost2;
                B(slotOut, 0x31F) = B(actor, 0x38D);
            }
        }
        return;
    }

    if (W(Scratchpad, 0x2B0) == 1) {
        signed short refX;
        signed short refZ;
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);

        if (S(ball, 0x3CC) >= 2) {
            refX = S(Scratchpad, 0xFE);
            refZ = S(Scratchpad, 0x100);
        } else {
            refX = S(Scratchpad, 0x36);
            refZ = S(Scratchpad, 0x66);
        }

        bestPrimaryCost = 0xFFFFFFFFU;

        for (i = 0; i < 10; i++, actor += 1000) {
            unsigned char id;
            unsigned int cost1;
            unsigned int cost2;

            if (B(actor, 0x00) == 0) {
                continue;
            }

            id = B(actor, 0x38D);
            if (B(actor, 0x39A) != 1 && (id == B(Scratchpad, 0x240) || id == B(Scratchpad, 0x241))) {
                int dx = S(actor, 0x02) - refX;
                int dz = S(actor, 0x06) - refZ;
                cost1 = (unsigned int)((dx * dx + dz * dz) / 65536);
                cost2 = cost1;
            } else {
                signed short px = S(actor, 0x02);
                signed short pz = S(actor, 0x06);
                int dx1 = px - refX;
                int dz1 = pz - refZ;
                int dx2 = px - S(Scratchpad, 0x40);
                int dz2 = pz - S(Scratchpad, 0x70);
                unsigned char mode30B;

                if (id != B(Scratchpad, 0x2AA) && S((unsigned char *)W(Scratchpad, 0x2F4), 0x04) >= -0x1FF) {
                    int bdx = px - S(Scratchpad, 0x36);
                    int bdz = pz - S(Scratchpad, 0x66);
                    if (bdx * bdx + bdz * bdz <= 0x23FFF) {
                        B(Scratchpad, 0x3D1 + atk) = id;
                    }
                }

                cost1 = (unsigned int)(dx1 * dx1 + dz1 * dz1);
                cost2 = (unsigned int)(dx2 * dx2 + dz2 * dz2);

                mode30B = B(Scratchpad, 0x30B);
                if (mode30B < 2) {
                    unsigned char role = B(actor, 0x391);
                    if (role >= 17) {
                        if (role < 19 || (role >= 21 && role < 26)) {
                            cost1 += 0x240000U;
                        }
                    }
                }
            }

            if (cost1 <= bestPrimaryCost) {
                if (bestPrimarySecCost < bestSecondCost) {
                    bestSecondCost = bestPrimarySecCost;
                    B(slotOut, 0x31F) = B(slotOut, 0x31E);
                }
                bestPrimaryCost = cost1;
                bestPrimarySecCost = cost2;
                B(slotOut, 0x31E) = B(actor, 0x38D);
            } else if (cost2 <= bestSecondCost) {
                bestSecondCost = cost2;
                B(slotOut, 0x31F) = B(actor, 0x38D);
            }
        }
    } else {
        signed short refX;
        signed short refZ;

        if (S(Scratchpad, 0x2B4) == 0x7FFF) {
            refX = S(Scratchpad, 0x36);
            refZ = S(Scratchpad, 0x66);
        } else {
            refX = S(Scratchpad, 0x2B4);
            refZ = S(Scratchpad, 0x2B6);
        }

        bestPrimaryCost = 0xFFFFFFFFU;

        for (i = 0; i < 10; i++, actor += 1000) {
            unsigned char id;
            unsigned int cost;

            if (B(actor, 0x00) == 0) {
                continue;
            }

            id = B(actor, 0x38D);
            if (id != B(Scratchpad, 0x2AA) && S((unsigned char *)W(Scratchpad, 0x2F4), 0x04) >= -0x1FF) {
                int bdx = S(actor, 0x02) - S(Scratchpad, 0x36);
                int bdz = S(actor, 0x06) - S(Scratchpad, 0x66);
                if (bdx * bdx + bdz * bdz <= 0x23FFF) {
                    B(Scratchpad, 0x3D1 + atk) = id;
                }
            }

            if (S(Scratchpad, 0x2B4) != 0x7FFF) {
                continue;
            }

            if (B(actor, 0x39A) != 1 && (id == B(Scratchpad, 0x240) || id == B(Scratchpad, 0x241))) {
                int dx = S(actor, 0x02) - refX;
                int dz = S(actor, 0x06) - refZ;
                cost = (unsigned int)((dx * dx + dz * dz) / 65536);
            } else {
                int dx = S(actor, 0x02) - refX;
                int dz = S(actor, 0x06) - refZ;
                cost = (unsigned int)(dx * dx + dz * dz);
                if (id != B(slotOut, 0x31E)) {
                    cost += 0x100000U;
                }
            }

            if (cost <= bestPrimaryCost) {
                bestSecondCost = bestPrimaryCost;
                B(slotOut, 0x31F) = B(slotOut, 0x31E);
                bestPrimaryCost = cost;
                B(slotOut, 0x31E) = B(actor, 0x38D);
            } else if (cost <= bestSecondCost) {
                bestSecondCost = cost;
                B(slotOut, 0x31F) = B(actor, 0x38D);
            }
        }
    }
}
