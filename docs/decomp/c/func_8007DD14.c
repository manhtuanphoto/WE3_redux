/*
 * func_8007DD14
 *
 * Purpose:
 *   Per-frame post-goal celebration and reset state machine for a player actor
 *   (dispatched by actor->substate +0x397). Handles goal-scorer celebrations
 *   (runs toward corner/touchline, slides, jumps, kneels), teammate reactions
 *   (chasing/mobbing the scorer or returning to formation), goalkeeper/opponent
 *   disappointment animations, and transitioning to post-celebration states
 *   (state 0x2E / substate 0x10/0x11 or 0xFF).
 *
 * Arguments:
 *   actor - pointer to 1000-byte player actor struct
 *
 * Return value:
 *   void
 *
 * Scratchpad fields read/written:
 *   +0xF8  (s16) target X passed to steering helper (W)
 *   +0xFC  (s16) target Z passed to steering helper (W)
 *   +0x29A (u8)  match phase (checked == 0xA, likely golden-goal/PK/replay) (R)
 *   +0x2AD (u8)  focus/goal-scorer actor id (R)
 *   +0x2DF (u8)  mode/flag checked during disappointment state 0x71 (R)
 *   +0x2F4 (u32) pointer to ball actor (R)
 *   +0x2FD (u8[2]) formation index per team side (R)
 *
 * Actor fields read/written:
 *   +0x00  (u8)  active flag (R on teammates)
 *   +0x02  (s16) X position (R/W)
 *   +0x06  (s16) Z position (R/W)
 *   +0x08  (s32) X velocity (W)
 *   +0x10  (s32) Z velocity (W)
 *   +0x38D (u8)  actor id (R)
 *   +0x38E (u16) current anim id (R)
 *   +0x390 (u8)  anim frame (R/W)
 *   +0x396 (u8)  state (R on focus actor, W = 0x2E on exit)
 *   +0x397 (u8)  substate (R/W)
 *   +0x398 (u8)  side 0/1 (R)
 *   +0x399 (u8)  attack direction flag (R)
 *   +0x39C (u8)  action/lock flag (W = 1)
 *   +0x3A0 (u32) speed (R/W)
 *   +0x3A4 (u8)  move heading (R/W)
 *   +0x3AA (u8)  body heading (R/W)
 *   +0x3AB (u8)  target heading (R/W)
 *   +0x3AC (u8)  mirror/foot flag (W)
 *   +0x3B0 (u8)  celebration/wait timer in frames (R/W)
 *   +0x3B4 (u8)  max speed (R)
 *   +0x3C4 (s16) target X (R/W)
 *   +0x3C6 (s16) target Z (R/W)
 *   +0x3C8 (s16) celebration/contact status flag (R/W)
 *   +0x3D2 (u8)  teammate join/follow flag (R/W)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x3320 (13088) = 52.352 m (goal-line X target)
 *   0x32E0 (13024) = 52.096 m (near goal-line X threshold)
 *   0x3270 (12912) = 51.648 m (corner-flag X target)
 *   0x2100 (8448)  = 33.792 m (touchline Z)
 *   0x1C81 (7297)  = 29.188 m, 0x1A81 (6785) = 27.14 m (touchline Z thresholds)
 *   0x3B66 (15206) = 60.824 m (off-pitch bench/exit Z target)
 *   0x3FFFFF dist^2 -> sqrt ~2048 = 8.192 m (teammate proximity to scorer)
 *   0xF618F  dist^2 -> sqrt ~1004 = 4.016 m (teammate approach radius)
 *   0x168 (360)    = 1.44 m (stop-short offset behind target in substate 0x48)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[]; /* 24 * 1000-byte actor pool */
extern unsigned char D_800CA5C0[]; /* 3-entry substate table: {0x58, 0x60, 0x68} */
extern unsigned char D_800CA5A8[]; /* frame-to-speed table indexed by anim frame */
extern unsigned char D_800C6BA8[]; /* frame lookup table for substate 0x10 */
extern signed short D_800C7DF8[][10][2]; /* formation base X offsets */
extern signed short D_800C7DFA[][10][2]; /* formation base Z offsets (+2 from D_800C7DF8) */
extern signed short D_800F3566;  /* packed celebration style/mode selector */
extern unsigned char D_800E6D5A; /* celebration override flag (return to formation) */
extern unsigned char D_800E6D5B; /* scorer celebration variant flag */
extern unsigned char D_800E6D5C; /* second celebrating teammate actor id */
extern unsigned char D_8010A323; /* match-state/skip-celebration flag */

extern int func_8001E6B8(int);
extern int func_8005C7A0(int);
extern int func_8005B36C(int, int, int, int);
extern int func_8005B7CC(int, int, int);
extern int func_8001E694(int, int);
extern int func_8001E5A4(int, int);
extern void func_8005BA30(unsigned char *, int, int);
extern void func_8005BA78(unsigned char *, int, int);
extern void func_8005B908(unsigned char *);
extern int func_8005BD1C(unsigned char *);
extern int func_8005CE14(int);
extern unsigned int func_800526C8(int);
extern void func_800479B8(unsigned char *, int);
extern void func_800498AC(unsigned char *);
extern int func_800AD678(void);
extern unsigned int func_800B1128(unsigned int);
extern void func_800807B0(unsigned char *, int);
extern void func_800807F0(unsigned char *);
extern unsigned char func_80081210(unsigned char *);
extern void func_80081374(unsigned char *);
extern void func_800814EC(unsigned char *);
extern void func_800818DC(unsigned char *);
extern void func_80081A6C(unsigned char *, signed short, signed short);

void func_8007DD14(unsigned char *actor)
{
    unsigned char *focusActor;
    unsigned char substate;

    focusActor = D_800FF748 + ((B(Scratchpad, 0x2AD) % 24) * 1000);
    substate = B(actor, 0x397);

    if (substate >= 0xDA) {
        func_8005B908(actor);
        return;
    }

    switch (substate) {
    case 0x00:
    {
        unsigned char actorId;

        H(actor, 0x3C8) = 0;
        if (D_8010A323 != 0) {
            if ((unsigned char)func_800526C8(B(Scratchpad, 0x2AD)) ==
                (unsigned char)func_800526C8(B(actor, 0x38D))) {
                B(actor, 0x397) = 0x70;
            } else {
                B(actor, 0x397) = D_800CA5C0[func_800AD678() % 3];
            }
            break;
        }

        actorId = B(actor, 0x38D);
        if (actorId == B(Scratchpad, 0x2AD)) {
            unsigned char *ball;
            signed short targetX;
            int productX;
            int randVal;

            B(actor, 0x3AB) = (unsigned char)func_8005B36C(S(actor, 2), S(actor, 6), 0, 0);
            targetX = (B(actor, 0x399) != 0) ? 0x3320 : -0x3320; /* +/- 52.352 m */
            S(actor, 0x3C4) = targetX;
            S(actor, 0x3C6) = 0;
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            productX = (int)S(ball, 2) * (int)S(actor, 2);
            randVal = func_800AD678();
            if (productX <= 0) {
                B(actor, 0x397) = D_800CA5C0[randVal % 3];
            } else if (D_800E6D5B != 0) {
                func_8005BA30(actor, 5, 0);
                H(actor, 0x3C8) = 1;
                W(actor, 0x3A0) = 0;
                B(actor, 0x397) = 0xD8;
            } else {
                B(actor, 0x397)++;
            }
            break;
        }

        if (B(actor, 0x399) != B(focusActor, 0x399)) {
            B(actor, 0x397) = 0x70;
            break;
        }

        /* Goalkeepers (actor id 1 and 12) */
        if (actorId == 1 || actorId == 12) {
            B(actor, 0x397) = D_800CA5C0[func_800AD678() % 3];
            break;
        }

        if (B(focusActor, 0x396) != 0x33) {
            int dx = S(actor, 2) - S(focusActor, 2);
            int dz = S(actor, 6) - S(focusActor, 6);
            int distSq = dx * dx + dz * dz;
            int randVal = func_800AD678();
            if (distSq <= 0x3FFFFF) { /* <= 8.192 m */
                B(actor, 0x397) = D_800CA5C0[randVal % 3];
                break;
            }
        }

        if (D_800E6D5A != 0 || B(actor, 0x38D) == D_800E6D5C) {
            W(actor, 0x3A0) = 0;
            B(actor, 0x397) = 0xC0;
            B(actor, 0x3B0) = (unsigned char)(func_800AD678() % 10);
            break;
        }

        {
            unsigned char angleToScorer;
            int teamBase;
            int angleSmall;
            unsigned char *teammate;
            int i;

            angleToScorer = (unsigned char)func_8005B36C(
                S(actor, 2), S(actor, 6), S(focusActor, 2), S(focusActor, 6));
            teamBase = func_8005CE14(B(actor, 0x38D));
            angleSmall = (angleToScorer < 0x60);
            teammate = D_800FF748 + (((unsigned char)teamBase % 24) * 1000);

            for (i = 0; i < 11; i++, teammate += 1000) {
                unsigned char mateHeading;
                unsigned int diff;
                int aligned;

                if (B(teammate, 0) == 0 || actor == teammate || S(teammate, 0x3C8) == 1) {
                    continue;
                }
                mateHeading = B(teammate, 0x3AA);
                diff = (unsigned int)(angleToScorer - mateHeading) & 0xFF;
                if (diff < 0x81) {
                    aligned = ((int)diff < 0x19);
                } else {
                    aligned = ((int)((unsigned int)(mateHeading - angleToScorer) & 0xFF) < 0x19);
                }
                if (aligned && !angleSmall) {
                    H(actor, 0x3C8) = 1;
                    B(actor, 0x397) = D_800CA5C0[func_800AD678() % 3];
                    return;
                }
            }

            if (D_800E6D5B != 0 || func_8001E6B8(3) == 0) {
                B(actor, 0x397) = D_800CA5C0[func_800AD678() % 3];
            } else {
                W(actor, 0x3A0) = 0;
                B(actor, 0x397) = 0xC0;
                B(actor, 0x3B0) = (unsigned char)((func_800AD678() % 10) + 8);
            }
        }
        break;
    }

    case 0x01:
        if ((D_800F3566 / 128) != 0) {
            int absX;

            func_8005BA78(actor, 2, B(actor, 0x390));
            if (S(actor, 6) < 0) {
                B(actor, 0x3AC) = B(actor, 0x399) ^ 1;
            } else {
                B(actor, 0x3AC) = B(actor, 0x399);
            }
            absX = S(actor, 2);
            if (absX < 0) {
                absX = -absX;
            }
            if (absX < 0x32E0) { /* 52.096 m */
                func_800814EC(actor);
                B(actor, 0x397) = 9;
            } else {
                B(actor, 0x397) = 8;
            }
        } else {
            func_8005BA78(actor, 2, B(actor, 0x390));
            B(actor, 0x397) = 0x18;
        }
        break;

    case 0x08:
    {
        int absX;

        func_8005BD1C(actor);
        S(Scratchpad, 0xF8) = S(actor, 0x3C4);
        S(Scratchpad, 0xFC) = S(actor, 0x3C6);
        func_800479B8(actor, (((D_800F3566 / 16) & 7) == 0) ? 1 : 0);
        absX = S(actor, 2);
        if (absX < 0) {
            absX = -absX;
        }
        if (absX < 0x32E0) {
            func_800814EC(actor);
            B(actor, 0x397)++;
        }
        func_800498AC(actor);
        break;
    }

    case 0x09:
    {
        signed short mode;

        func_8005BD1C(actor);
        S(Scratchpad, 0xF8) = S(actor, 0x3C4);
        S(Scratchpad, 0xFC) = S(actor, 0x3C6);
        func_800479B8(actor, (((D_800F3566 / 16) & 7) == 0) ? 1 : 0);
        mode = (signed short)((D_800F3566 / 16) % 8);
        if (mode == 0) {
            if (W(actor, 0x3A0) == B(actor, 0x3B4)) {
                B(actor, 0x397) = 0x10;
            }
        } else if (mode == 2) {
            if (W(actor, 0x3A0) == 0x30) {
                B(actor, 0x3B0) = 1;
                B(actor, 0x397) = 0x28;
            }
        } else {
            if (W(actor, 0x3A0) == 0x30) {
                B(actor, 0x3B0) = 1;
                B(actor, 0x397) = 0x20;
            }
        }
        func_800498AC(actor);
        break;
    }

    case 0x10:
        func_8005BD1C(actor);
        func_800807B0(actor, 8);
        if ((signed short)(D_800F3566 % 16) == 4) {
            int absZ = S(actor, 6);
            if (absZ < 0) {
                absZ = -absZ;
            }
            if (absZ >= 0x1A81) { /* 27.14 m */
                B(actor, 0x397) = 0x50;
            }
        } else {
            int absZ = S(actor, 6);
            if (absZ < 0) {
                absZ = -absZ;
            }
            if (absZ >= 0x1C81) { /* 29.188 m */
                func_8005BA30(actor, 2, D_800C6BA8[B(actor, 0x390)]);
                B(actor, 0x397) = 0x30;
            }
        }
        func_80081374(actor);
        break;

    case 0x18:
        func_8005BD1C(actor);
        S(Scratchpad, 0xF8) = (B(actor, 0x399) != 0) ? 0x3320 : -0x3320;
        S(Scratchpad, 0xFC) = 0;
        func_800479B8(actor, 0);
        if (W(actor, 0x3A0) == 0x30) {
            signed short mode = (signed short)((D_800F3566 / 16) % 8);
            if (mode == 1) {
                B(actor, 0x397) = 0x20;
            } else if (mode == 2) {
                B(actor, 0x3B0) = 1;
                B(actor, 0x397) = 0x28;
            } else if (mode == 3) {
                B(actor, 0x3B0) = 1;
                B(actor, 0x397) = 0x40;
            }
        }
        func_800498AC(actor);
        break;

    case 0x20:
        func_800807B0(actor, 8);
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0xD) {
            func_8005BA30(actor, 0x3A, 0);
            if ((D_800F3566 / 128) == 0) {
                signed short rem = (signed short)(D_800F3566 % 16);
                if (rem == 1) {
                    B(actor, 0x3B0) = 6;
                } else if (rem <= 0) {
                    B(actor, 0x3B0) = 4;
                } else if (rem < 4) {
                    B(actor, 0x3B0) = 2;
                } else {
                    B(actor, 0x3B0) = 4;
                }
            }
            B(actor, 0x397)++;
        }
        func_80081374(actor);
        break;

    case 0x21:
        func_800807B0(actor, 8);
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0x15) {
            if ((D_800F3566 / 128) != 0) {
                signed short rem = (signed short)(D_800F3566 % 16);
                int margin;
                int absZ;

                if (rem < 0) {
                    margin = 0x700;
                } else if (rem < 2) {
                    margin = 0xB00;
                } else if (rem == 4) {
                    margin = 0x980;
                } else {
                    margin = 0x700;
                }
                absZ = S(actor, 6);
                if (absZ < 0) {
                    absZ = -absZ;
                }
                if (0x2100 - margin < absZ) {
                    B(actor, 0x397)++;
                } else {
                    func_8005BA30(actor, 0x3A, 4);
                }
            } else {
                unsigned char timer = B(actor, 0x3B0);
                B(actor, 0x3B0) = (unsigned char)(timer - 1);
                if (timer == 0) {
                    B(actor, 0x397)++;
                } else {
                    func_8005BA30(actor, 0x3A, 4);
                }
            }
        }
        func_80081374(actor);
        break;

    case 0x22:
        func_800807B0(actor, 8);
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0x19) {
            func_8005BA30(actor, 2, 4);
            B(actor, 0x397) = 0x30;
        }
        func_80081374(actor);
        break;

    case 0x24:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 5) {
            func_8005BA30(actor, 0x3B, 0);
            W(actor, 0x3A0) = 0x30;
            B(actor, 0x3B0) = 3;
            B(actor, 0x397)++;
        }
        func_80081374(actor);
        func_800807B0(actor, 2);
        break;

    case 0x25:
    {
        unsigned char timer;
        func_8005BD1C(actor);
        timer = B(actor, 0x3B0);
        B(actor, 0x3B0) = (unsigned char)(timer - 1);
        if (timer != 0) {
            if (B(actor, 0x390) == 0x1D) {
                func_8005BA30(actor, 0x3B, 0xC);
            }
        } else {
            if (B(actor, 0x390) == 0x29) {
                func_8005BA30(actor, 2, 0x10);
            }
        }
        B(actor, 0x3AB) = (unsigned char)((B(actor, 0x399) ^ 1) << 7);
        func_80081374(actor);
        func_800807B0(actor, 2);
        break;
    }

    case 0x28:
        func_8005BD1C(actor);
        func_800807B0(actor, 8);
        if (B(actor, 0x390) == 6) {
            unsigned char timer = B(actor, 0x3B0);
            B(actor, 0x3B0) = (unsigned char)(timer - 1);
            if (timer == 0) {
                func_8005BA30(actor, 0x3D, 0);
                if (S(actor, 6) < 0) {
                    B(actor, 0x3AC) = B(actor, 0x399) ^ 1;
                } else {
                    B(actor, 0x3AC) = B(actor, 0x399);
                }
                if ((D_800F3566 / 128) == 0) {
                    signed short rem = (signed short)(D_800F3566 % 16);
                    if (rem == 1) {
                        B(actor, 0x3B0) = 5;
                    } else if (rem <= 0) {
                        B(actor, 0x3B0) = 3;
                    } else if (rem < 4) {
                        B(actor, 0x3B0) = 1;
                    } else {
                        B(actor, 0x3B0) = 3;
                    }
                }
                B(actor, 0x397)++;
            }
        }
        func_80081374(actor);
        break;

    case 0x29:
        func_8005BD1C(actor);
        func_800807B0(actor, 8);
        if (B(actor, 0x390) == 0x23) {
            if ((D_800F3566 / 128) != 0) {
                signed short rem = (signed short)(D_800F3566 % 16);
                int margin;
                int absZ;

                if (rem < 0) {
                    margin = 0x700;
                } else if (rem < 2) {
                    margin = 0xB00;
                } else if (rem == 4) {
                    margin = 0x980;
                } else {
                    margin = 0x700;
                }
                absZ = S(actor, 6);
                if (absZ < 0) {
                    absZ = -absZ;
                }
                if (0x2100 - margin < absZ) {
                    B(actor, 0x397)++;
                } else {
                    func_8005BA30(actor, 0x3D, 0x12);
                }
            } else {
                unsigned char timer = B(actor, 0x3B0);
                B(actor, 0x3B0) = (unsigned char)(timer - 1);
                if (timer == 0) {
                    B(actor, 0x397)++;
                } else {
                    func_8005BA30(actor, 0x3D, 0x12);
                }
            }
        }
        func_80081374(actor);
        break;

    case 0x2A:
        func_8005BD1C(actor);
        func_800807B0(actor, 8);
        if (B(actor, 0x390) == 0x2E) {
            func_8005BA30(actor, 2, 0);
            B(actor, 0x397) = 0x30;
        }
        func_80081374(actor);
        break;

    case 0x30:
        func_8005BD1C(actor);
        func_800807B0(actor, 2);
        W(actor, 0x3A0) = 0x30;
        if ((D_800F3566 / 128) != 0) {
            signed short rem;
            if ((signed short)(D_800F3566 % 16) < 2) {
                signed short targetZ = (S(actor, 6) > 0) ? 0x2100 : -0x2100;
                B(actor, 0x3AB) = (unsigned char)func_8005B36C(
                    S(actor, 2), S(actor, 6), 0, targetZ);
            }
            rem = (signed short)(D_800F3566 % 16);
            switch ((unsigned int)rem) {
            case 0:
                B(actor, 0x397) = 0x24;
                break;
            case 1:
                B(actor, 0x3B0) = 1;
                B(actor, 0x397) = 0x40;
                break;
            case 2:
                B(actor, 0x397) = 0x3C;
                break;
            case 3:
                B(actor, 0x397) = 0x34;
                break;
            case 4:
                B(actor, 0x397) = 0x50;
                break;
            default:
                B(actor, 0x397) = 0xA8;
                break;
            }
        } else {
            signed short rem = (signed short)(D_800F3566 % 16);
            if (rem == 1) {
                B(actor, 0x397) = 0x38;
            } else if (rem >= 2 && rem < 4) {
                B(actor, 0x397) = 0x48;
            } else {
                B(actor, 0x3B0) = 1;
                B(actor, 0x397) = 0x40;
            }
        }
        break;

    case 0x34:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 2) {
            func_8005BA30(actor, 0x3F, 0);
            B(actor, 0x3B0) = 3;
            H(actor, 0x3C8) = 1;
            B(actor, 0x397)++;
        }
        func_800807B0(actor, 8);
        break;

    case 0x35:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0x2E) {
            unsigned char timer = B(actor, 0x3B0);
            B(actor, 0x3B0) = (unsigned char)(timer - 1);
            if (timer != 0) {
                B(actor, 0x397)++;
            } else {
                B(actor, 0x397) = 0x37;
            }
        }
        if (((D_800F3566 / 16) & 7) == 0) {
            if (W(actor, 0x3A0) < 2) {
                W(actor, 0x3A0) = 0;
            } else {
                W(actor, 0x3A0) -= 1;
            }
        } else {
            if (W(actor, 0x3A0) < 3) {
                W(actor, 0x3A0) = 0;
            } else {
                W(actor, 0x3A0) -= 2;
            }
        }
        B(actor, 0x39C) = 1;
        break;

    case 0x36:
    {
        unsigned char frame = (unsigned char)(B(actor, 0x390) - 1);
        B(actor, 0x390) = frame;
        func_8005BA30(actor, 0x3F, frame);
        if (B(actor, 0x390) == 0x28) {
            B(actor, 0x397)--;
        }
        B(actor, 0x39C) = 1;
        break;
    }

    case 0x37:
        B(actor, 0x39C) = 1;
        break;

    case 0x38:
        func_8005BD1C(actor);
        if (W(actor, 0x3A0) < 3) {
            W(actor, 0x3A0) = 0;
        } else {
            W(actor, 0x3A0) -= 2;
        }
        if (W(actor, 0x3A0) < 13) {
            if (H(actor, 0x38E) != 0x35) {
                func_8005BA30(actor, 0x35, 0);
            }
            H(actor, 0x3C8) = 1;
            W(actor, 0x3A0) = 0;
            B(actor, 0x397)++;
        }
        func_800807B0(actor, 8);
        break;

    case 0x39:
        func_8005BA30(actor, 0x42, 0);
        B(actor, 0x397) = 0x5A;
        break;

    case 0x3C:
        func_8005BD1C(actor);
        if (W(actor, 0x3A0) < 3) {
            W(actor, 0x3A0) = 0;
        } else {
            W(actor, 0x3A0) -= 2;
        }
        if (W(actor, 0x3A0) < 13) {
            if (H(actor, 0x38E) != 0x35) {
                func_8005BA30(actor, 0x35, 0);
            }
            W(actor, 0x3A0) = 0;
            B(actor, 0x397)++;
        }
        func_800807B0(actor, 8);
        break;

    case 0x3D:
        func_8005BD1C(actor);
        func_8005BA30(actor, 0x3E, 0);
        if (S(actor, 6) >= 0) {
            B(actor, 0x3AB) = (unsigned char)func_8005B36C(
                S(actor, 2), S(actor, 6), 0, S(actor, 6));
            B(actor, 0x3AC) = B(actor, 0x399);
        } else {
            B(actor, 0x3AB) = (unsigned char)func_8005B36C(
                S(actor, 2), S(actor, 6), 0, S(actor, 6));
            B(actor, 0x3AC) = B(actor, 0x399) ^ 1;
        }
        B(actor, 0x3AA) = (S(actor, 6) <= 0) ? 0xC0 : 0x40;
        H(actor, 0x3C8) = 4;
        B(actor, 0x397)++;
        break;

    case 0x3E:
        func_8005BD1C(actor);
        B(actor, 0x3A4) = (unsigned char)func_8005B7CC(
            B(actor, 0x3AB), B(actor, 0x3A4), 8);
        W(actor, 0x3A0) = (unsigned int)((D_800CA5A8[B(actor, 0x390)] * 112) / 100);
        if (B(actor, 0x390) == 0x16) {
            func_8005BA30(actor, 0x3E, 6);
        }
        break;

    case 0x40:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 2) {
            if (B(actor, 0x3B0) != 0) {
                B(actor, 0x3B0)--;
            } else {
                func_8005BA30(actor, 0x41, 0);
                B(actor, 0x397)++;
            }
        }
        func_80081374(actor);
        func_800807B0(actor, 2);
        break;

    case 0x41:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0x20) {
            func_8005BA30(actor, 2, 3);
            if ((D_800F3566 / 128) == 0 && (signed short)((D_800F3566 / 16) % 8) == 3) {
                B(actor, 0x397) = 0x30;
            } else {
                B(actor, 0x397) = 0x24;
            }
        }
        func_80081374(actor);
        func_800807B0(actor, 2);
        break;

    case 0x48:
        func_8005BD1C(actor);
        if (D_800E6D5C != 0) {
            unsigned char *target = D_800FF748 + ((D_800E6D5C % 24) * 1000);
            int dx, dz, distSq;
            unsigned int dist;

            B(actor, 0x3AB) = (unsigned char)func_8005B36C(
                S(actor, 2), S(actor, 6), S(target, 2), S(target, 6));
            dx = S(actor, 2) - S(target, 2);
            dz = S(actor, 6) - S(target, 6);
            distSq = dx * dx + dz * dz;
            dist = func_800B1128((unsigned int)distSq);
            if (distSq <= 0xF618F) { /* <= ~4.016 m */
                unsigned short stopDist = (unsigned short)((dist >> 1) - 0x168); /* - 1.44 m */
                H(actor, 0x3C4) = (unsigned short)(H(actor, 2) + func_8001E694(B(actor, 0x3AB), stopDist));
                H(actor, 0x3C8) = 2;
                B(actor, 0x3D2) = 1;
                H(actor, 0x3C6) = (unsigned short)(H(actor, 6) + func_8001E5A4(B(actor, 0x3AB), stopDist));
                B(actor, 0x397)++;
            }
        }
        func_800807B0(actor, 8);
        break;

    case 0x49:
    {
        unsigned short anim;
        unsigned char targetId;
        unsigned char *target;

        func_8005BD1C(actor);
        func_80081A6C(actor, S(actor, 0x3C4), S(actor, 0x3C6));
        anim = H(actor, 0x38E);
        if (anim == 1 || anim == 0xE) {
            if (anim != 0x25) {
                func_8005BA30(actor, 0x25, 0);
            }
            B(actor, 0x3AC) = 0;
            W(actor, 0x3A0) = 0x10;
            B(actor, 0x397)++;
        }
        B(actor, 0x3D2) = 1;
        if (B(actor, 0x38D) == B(Scratchpad, 0x2AD)) {
            targetId = D_800E6D5C;
        } else {
            targetId = B(Scratchpad, 0x2AD);
        }
        target = D_800FF748 + ((targetId % 24) * 1000);
        B(actor, 0x3AB) = (unsigned char)func_8005B36C(
            S(actor, 2), S(actor, 6), S(target, 2), S(target, 6));
        func_800807B0(actor, 12);
        break;
    }

    case 0x4A:
    {
        unsigned char targetId;
        unsigned char *target;

        func_8005BD1C(actor);
        B(actor, 0x3D2) = 1;
        if (B(actor, 0x390) == 0x15) {
            if (B(actor, 0x38D) == D_800E6D5C) {
                H(actor, 0x3C8) = 2;
            }
            W(actor, 0x3A0) = 0;
            B(actor, 0x397)++;
        }
        if (B(actor, 0x38D) == B(Scratchpad, 0x2AD)) {
            targetId = D_800E6D5C;
        } else {
            targetId = B(Scratchpad, 0x2AD);
        }
        target = D_800FF748 + ((targetId % 24) * 1000);
        B(actor, 0x3AB) = (unsigned char)func_8005B36C(
            S(actor, 2), S(actor, 6), S(target, 2), S(target, 6));
        func_800807B0(actor, 8);
        break;
    }

    case 0x4B:
        B(actor, 0x3D2) = 1;
        break;

    case 0x4D:
    {
        unsigned short anim;

        func_8005BD1C(actor);
        func_80081A6C(actor, S(actor, 0x3C4), S(actor, 0x3C6));
        anim = H(actor, 0x38E);
        if (anim == 1 || anim == 0xE) {
            unsigned char *partner = D_800FF748 + ((D_800E6D5C % 24) * 1000);
            if (S(partner, 0x3C8) == 2) {
                if (anim != 0x25) {
                    func_8005BA30(actor, 0x25, 0);
                }
                B(actor, 0x3AC) = 0;
                W(actor, 0x3A0) = 0x10;
                B(actor, 0x397)++;
            }
        }
        B(actor, 0x3D2) = 1;
        func_800807B0(actor, 12);
        break;
    }

    case 0x4E:
        func_8005BD1C(actor);
        B(actor, 0x3D2) = 1;
        if (B(actor, 0x390) == 0x10) {
            H(actor, 0x3C8) = 2;
            W(actor, 0x3A0) = 0;
            B(actor, 0x397) = 0x4B;
        }
        func_800807B0(actor, 16);
        break;

    case 0x50:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 2) {
            func_8005BA30(actor, 0x52, 0);
            B(actor, 0x39C) = 1;
            H(actor, 0x3C8) = 3;
            B(actor, 0x397)++;
        }
        break;

    case 0x51:
        W(actor, 0x3A0) = (W(actor, 0x3A0) * 98U) / 100U;
        B(actor, 0x39C) = 1;
        if (B(actor, 0x390) == 0x26) {
            if (W(actor, 0x3A0) == 0) {
                B(actor, 0x3B0) = (unsigned char)(func_8001E6B8(32) + 16);
                B(actor, 0x397)++;
            }
        } else {
            func_8005BD1C(actor);
        }
        break;

    case 0x52:
        B(actor, 0x39C) = 1;
        B(actor, 0x3B0)--;
        if (B(actor, 0x3B0) == 0) {
            func_8005BA30(actor, 0x21, 0);
            B(actor, 0x397)++;
        }
        break;

    case 0x53:
        func_8005BD1C(actor);
        B(actor, 0x39C) = 1;
        if (B(actor, 0x390) == 0x12) {
            if (B(actor, 0x38D) == B(Scratchpad, 0x2AD)) {
                B(actor, 0x397) = 0x38;
            } else {
                B(actor, 0x397) = D_800CA5C0[func_800AD678() % 3];
            }
        }
        break;

    case 0x58:
        func_8005BA30(actor, 0x35, 0);
        B(actor, 0x3B0) = (unsigned char)(func_8001E6B8(16) + 4);
        W(actor, 0x3A0) = 0;
        B(actor, 0x397)++;
        break;

    case 0x59:
        if (B(actor, 0x3B0) != 0) {
            B(actor, 0x3B0)--;
        } else {
            func_8005BA30(actor, 0x42, 0);
            B(actor, 0x3B0) = (unsigned char)(func_8001E6B8(6) + 6);
            B(actor, 0x397)++;
        }
        if (B(actor, 0x38D) != B(focusActor, 0x38D)) {
            B(actor, 0x3AB) = (unsigned char)func_8005B36C(
                S(actor, 2), S(actor, 6), S(focusActor, 2), S(focusActor, 6));
        }
        func_800818DC(actor);
        func_800807B0(actor, 8);
        break;

    case 0x5A:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0x11) {
            unsigned char timer = B(actor, 0x3B0);
            B(actor, 0x3B0) = (unsigned char)(timer - 1);
            if (timer == 0) {
                B(actor, 0x397) = 0x5C;
            } else {
                B(actor, 0x397)++;
            }
        }
        if (B(actor, 0x38D) != B(focusActor, 0x38D)) {
            B(actor, 0x3AB) = (unsigned char)func_8005B36C(
                S(actor, 2), S(actor, 6), S(focusActor, 2), S(focusActor, 6));
        }
        func_800818DC(actor);
        func_800807B0(actor, 8);
        break;

    case 0x5B:
    {
        unsigned char frame = (unsigned char)(B(actor, 0x390) - 1);
        B(actor, 0x390) = frame;
        func_8005BA30(actor, 0x42, frame);
        if (B(actor, 0x390) == 0xB) {
            B(actor, 0x397)--;
        }
        if (B(actor, 0x38D) != B(focusActor, 0x38D)) {
            B(actor, 0x3AB) = (unsigned char)func_8005B36C(
                S(actor, 2), S(actor, 6), S(focusActor, 2), S(focusActor, 6));
        }
        func_800818DC(actor);
        func_800807B0(actor, 8);
        break;
    }

    case 0x5C:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0x20) {
            if (B(Scratchpad, 0x29A) == 0xA) {
                if (H(actor, 0x38E) != 0x35) {
                    func_8005BA30(actor, 0x35, 0);
                }
                B(actor, 0x397) = 0xD0;
            } else {
                B(actor, 0x397) = 0x58;
            }
        }
        func_800818DC(actor);
        break;

    case 0x60:
        func_8005BA30(actor, 0x35, 0);
        B(actor, 0x3B0) = (unsigned char)(func_8001E6B8(16) + 2);
        W(actor, 0x3A0) = 0;
        B(actor, 0x397)++;
        break;

    case 0x61:
        if (B(actor, 0x3B0) != 0) {
            B(actor, 0x3B0)--;
        } else {
            func_8005BA30(actor, 0x44, 0);
            if (func_8001E6B8(2) != 0) {
                B(actor, 0x397) = 0x63;
            } else {
                B(actor, 0x397)++;
            }
        }
        func_800818DC(actor);
        break;

    case 0x62:
        if (B(actor, 0x3B0) == 0) {
            unsigned char frame = (unsigned char)(B(actor, 0x390) + 1);
            B(actor, 0x390) = frame;
            if (frame == 0x14) {
                B(actor, 0x3B0) = 1;
            }
        } else {
            unsigned char frame = (unsigned char)(B(actor, 0x390) - 1);
            B(actor, 0x390) = frame;
            if (frame == 8) {
                B(actor, 0x3B0) = 0;
            }
        }
        func_8005BA30(actor, 0x44, B(actor, 0x390));
        if (B(actor, 0x38D) != B(focusActor, 0x38D)) {
            B(actor, 0x3AB) = (unsigned char)func_8005B36C(
                S(actor, 2), S(actor, 6), S(focusActor, 2), S(focusActor, 6));
        }
        func_800807B0(actor, 8);
        func_800818DC(actor);
        break;

    case 0x63:
        if (B(actor, 0x3B0) == 0) {
            unsigned char frame = (unsigned char)(B(actor, 0x390) + 1);
            B(actor, 0x390) = frame;
            if (frame == 0x24) {
                B(actor, 0x3B0) = 1;
            }
        } else {
            unsigned char frame = (unsigned char)(B(actor, 0x390) - 1);
            B(actor, 0x390) = frame;
            if (frame == 0x18) {
                B(actor, 0x3B0) = 0;
            }
        }
        func_8005BA30(actor, 0x44, B(actor, 0x390));
        if (B(actor, 0x38D) != B(focusActor, 0x38D)) {
            B(actor, 0x3AB) = (unsigned char)func_8005B36C(
                S(actor, 2), S(actor, 6), S(focusActor, 2), S(focusActor, 6));
        }
        func_800807B0(actor, 8);
        func_800818DC(actor);
        break;

    case 0x68:
        func_8005BA30(actor, 0x35, 0);
        B(actor, 0x3B0) = (unsigned char)(func_8001E6B8(16) + 4);
        W(actor, 0x3A0) = 0;
        B(actor, 0x397)++;
        break;

    case 0x69:
        if (B(actor, 0x3B0) != 0) {
            B(actor, 0x3B0)--;
        } else {
            func_8005BA30(actor, 0x51, 0);
            B(actor, 0x3B0) = (unsigned char)(func_8001E6B8(4) + 8);
            B(actor, 0x397)++;
        }
        if (B(actor, 0x38D) != B(focusActor, 0x38D)) {
            B(actor, 0x3AB) = (unsigned char)func_8005B36C(
                S(actor, 2), S(actor, 6), S(focusActor, 2), S(focusActor, 6));
        }
        func_800818DC(actor);
        func_800807B0(actor, 8);
        break;

    case 0x6A:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0x18) {
            func_8005BA30(actor, 0x51, 0xE);
            if (B(Scratchpad, 0x29A) == 0xA) {
                unsigned char timer = B(actor, 0x3B0);
                B(actor, 0x3B0) = (unsigned char)(timer - 1);
                if (timer == 0) {
                    B(actor, 0x397)++;
                }
            }
        }
        if (B(actor, 0x38D) != B(focusActor, 0x38D)) {
            B(actor, 0x3AB) = (unsigned char)func_8005B36C(
                S(actor, 2), S(actor, 6), S(focusActor, 2), S(focusActor, 6));
        }
        func_800818DC(actor);
        func_800807B0(actor, 8);
        break;

    case 0x6B:
    {
        unsigned char frame = (unsigned char)(B(actor, 0x390) - 1);
        B(actor, 0x390) = frame;
        func_8005BA30(actor, 0x51, frame);
        if (B(actor, 0x390) == 0) {
            if (H(actor, 0x38E) != 0x35) {
                func_8005BA30(actor, 0x35, 0);
            }
            B(actor, 0x397) = 0xD0;
        }
        func_800818DC(actor);
        break;
    }

    case 0x70:
        if (H(actor, 0x38E) != 1) {
            W(actor, 0x3A0) = 0;
            func_8005BA30(actor, 1, 0);
        }
        B(actor, 0x3B0) = (unsigned char)(func_8005C7A0(10) + 30);
        B(actor, 0x397)++;
        break;

    case 0x71:
        func_8005BD1C(actor);
        B(actor, 0x3B0)--;
        if (B(actor, 0x3B0) != 0) {
            break;
        }
        if (B(Scratchpad, 0x2DF) != 0 && B(actor, 0x3D2) == 1 && func_8001E6B8(3) == 0) {
            func_8005BA30(actor, (func_8001E6B8(2) != 0) ? 0x46 : 0x47, 0);
            B(actor, 0x397) = 0xFF;
            break;
        }
        if (B(Scratchpad, 0x29A) != 0xA &&
            (B(actor, 0x38D) == 1 || B(actor, 0x38D) == 0xC)) {
            func_8005BA30(actor, 0x3C, 0);
            B(actor, 0x397) = 0xA0;
            break;
        }
        switch (func_8001E6B8(8)) {
        case 0:
        case 1:
        case 2:
            func_8005BA30(actor, 0x37, 0);
            B(actor, 0x397) = 0x88;
            break;
        case 3:
        case 4:
        case 5:
            func_8005BA30(actor, 0x38, 0);
            B(actor, 0x397) = 0x98;
            break;
        case 6:
        case 7:
            func_8005BA30(actor, 0x33, 0);
            B(actor, 0x397) = 0x80;
            break;
        default:
            break;
        }
        break;

    case 0x80:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0xF) {
            if (B(Scratchpad, 0x29A) != 0xA &&
                ((unsigned char)func_80081210(actor) != 0 || D_8010A323 != 0)) {
                B(actor, 0x397) = 0xFF;
            } else {
                B(actor, 0x3B0) = (unsigned char)((func_800AD678() % 32) + 0x78);
                B(actor, 0x397)++;
            }
        }
        break;

    case 0x81:
        if (B(actor, 0x3B0) != 0) {
            B(actor, 0x3B0)--;
            break;
        }
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0x1C) {
            if (B(Scratchpad, 0x29A) == 0xA) {
                if (H(actor, 0x38E) != 0x35) {
                    func_8005BA30(actor, 0x35, 0);
                }
                B(actor, 0x397) = 0xD0;
            } else {
                if (H(actor, 0x38E) != 1) {
                    func_8005BA30(actor, 1, (unsigned char)func_8001E6B8(0x15));
                }
                B(actor, 0x3B0) = (unsigned char)func_8001E6B8(8);
                B(actor, 0x397) = 0x90;
            }
        }
        break;

    case 0x88:
        func_8005BD1C(actor);
        if (B(actor, 0x390) < 8) {
            B(actor, 0x3A4) = (unsigned char)(B(actor, 0x3AA) - 0x40);
        } else {
            B(actor, 0x3A4) = (unsigned char)(B(actor, 0x3AA) + 0x40);
        }
        if (B(actor, 0x390) == 0x1B) {
            if (B(Scratchpad, 0x29A) != 0xA && (unsigned char)func_80081210(actor) != 0) {
                B(actor, 0x397) = 0xFF;
            } else {
                if (D_8010A323 != 0) {
                    B(actor, 0x3B0) = (unsigned char)((func_800AD678() % 48) - 16);
                } else {
                    B(actor, 0x3B0) = (unsigned char)((func_800AD678() % 80) + 120);
                }
                B(actor, 0x397)++;
            }
        }
        break;

    case 0x89:
        if (B(actor, 0x3B0) != 0) {
            B(actor, 0x3B0)--;
            break;
        }
        func_8005BA30(actor, 0x40, 0x25);
        if (B(Scratchpad, 0x29A) == 0xA) {
            B(actor, 0x397) = 0xD1;
        } else {
            B(actor, 0x397) = 0x91;
        }
        break;

    case 0x90:
        func_8005BD1C(actor);
        if (B(actor, 0x3B0) != 0) {
            B(actor, 0x3B0)--;
        } else {
            func_8005BA30(actor, 5, (unsigned char)func_8001E6B8(0x23));
            W(actor, 0x3A0) = 12;
            B(actor, 0x397)++;
        }
        break;

    case 0x91:
        if (D_8010A323 != 0) {
            B(actor, 0x3AB) = (unsigned char)func_8005B36C(
                S(actor, 2), S(actor, 6), 0, 0);
        } else {
            int slot = ((int)B(actor, 0x38D) - 1) % 11 - 1;
            int form = B(Scratchpad, 0x2FD + B(actor, 0x398));
            signed short fx = D_800C7DF8[form][slot][0];
            signed short fz = D_800C7DFA[form][slot][0];
            if (B(actor, 0x399) != 0) {
                fx = -fx;
                fz = -fz;
            }
            B(actor, 0x3AB) = (unsigned char)func_8005B36C(
                S(actor, 2), S(actor, 6), fx, fz);
        }
        B(actor, 0x397)++;
        break;

    case 0x92:
        if ((unsigned char)func_80081210(actor) != 0) {
            if (H(actor, 0x38E) == 0x40) {
                B(actor, 0x3AB) = (unsigned char)func_8005B36C(
                    S(actor, 2), S(actor, 6), 0, S(actor, 6));
                W(actor, 0x3A0) = 8;
                B(actor, 0x397)++;
            } else {
                if (H(actor, 0x38E) != 0x37) {
                    func_8005BA30(actor, 0x37, 0);
                }
                B(actor, 0x397) = 0x94;
                W(actor, 0x3A0) = 0;
            }
        } else {
            func_8005BD1C(actor);
            func_800807B0(actor, 4);
            W(actor, 0x3A0) += 6;
            if (W(actor, 0x3A0) >= 13) {
                W(actor, 0x3A0) = 12;
            }
        }
        break;

    case 0x93:
        func_800807B0(actor, 2);
        func_8005BD1C(actor);
        break;

    case 0x94:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0x1B) {
            B(actor, 0x397) = 0xFF;
        }
        break;

    case 0x98:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0xD) {
            if (B(Scratchpad, 0x29A) != 0xA &&
                ((unsigned char)func_80081210(actor) != 0 || D_8010A323 != 0)) {
                B(actor, 0x397) = 0xFF;
            } else {
                B(actor, 0x3B0) = (unsigned char)((func_800AD678() % 80) + 120);
                B(actor, 0x397)++;
            }
        }
        break;

    case 0x99:
        if (B(actor, 0x3B0) != 0) {
            B(actor, 0x3B0)--;
            break;
        }
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0x21) {
            if (B(Scratchpad, 0x29A) == 0xA) {
                if (H(actor, 0x38E) != 0x35) {
                    func_8005BA30(actor, 0x35, 0);
                }
                B(actor, 0x397) = 0xD0;
            } else {
                if (H(actor, 0x38E) != 1) {
                    func_8005BA30(actor, 1, (unsigned char)func_8001E6B8(0x15));
                }
                B(actor, 0x3B0) = (unsigned char)func_8001E6B8(8);
                B(actor, 0x397) = 0x90;
            }
        }
        break;

    case 0xA0:
        if (B(actor, 0x390) < 3) {
            W(actor, 0x3A0) = 4;
        } else {
            W(actor, 0x3A0) = 0;
        }
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0x2D) {
            B(actor, 0x397) = 0xFF;
        }
        break;

    case 0xA8:
    {
        signed short targetX = (S(actor, 2) <= 0) ? -0x3270 : 0x3270; /* +/- 51.648 m */
        signed short targetZ = (S(actor, 6) <= 0) ? -0x2100 : 0x2100; /* +/- 33.792 m */

        func_8005BD1C(actor);
        func_80081A6C(actor, targetX, targetZ);
        if (H(actor, 0x38E) == 1) {
            func_8005BA30(actor, 0x4E, 0);
            if (S(actor, 2) <= 0) {
                B(actor, 0x3AC) = (S(actor, 6) < 1) ? 1 : 0;
            } else {
                B(actor, 0x3AC) = (S(actor, 6) > 0) ? 1 : 0;
            }
            H(actor, 0x3C8) = 1;
            W(actor, 0x3A0) = 0;
            B(actor, 0x397)++;
        }
        break;
    }

    case 0xA9:
    {
        unsigned char targetAngle;

        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0xE) {
            B(actor, 0x3B0) = (unsigned char)(func_8001E6B8(32) + 64);
            B(actor, 0x397)++;
        }
        targetAngle = (S(actor, 6) <= 0) ? 0xC0 : 0x40;
        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(targetAngle, B(actor, 0x3AA), 4);
        break;
    }

    case 0xAA:
        if (B(actor, 0x3B0) != 0) {
            B(actor, 0x3B0)--;
        } else {
            unsigned char frame = B(actor, 0x390);
            if (frame == 0) {
                B(actor, 0x397) = 0x38;
            } else {
                frame--;
                B(actor, 0x390) = frame;
                func_8005BA30(actor, 0x4E, frame);
            }
        }
        break;

    case 0xC0:
        func_8005BD1C(actor);
        if (B(actor, 0x3B0) != 0) {
            B(actor, 0x3B0)--;
            break;
        }
        if (H(actor, 0x38E) != 2) {
            func_8005BA30(actor, 2, 0);
        }
        W(actor, 0x3A0) = 0x2C;
        if (D_800E6D5A != 0) {
            int slot = ((int)B(actor, 0x38D) - 1) % 11 - 1;
            int form = B(Scratchpad, 0x2FD + B(actor, 0x398));
            signed short fx = D_800C7DF8[form][slot][0];
            signed short fz = D_800C7DFA[form][slot][0];
            if (B(actor, 0x399) != 0) {
                fx = -fx;
                fz = -fz;
            }
            B(actor, 0x3AB) = (unsigned char)func_8005B36C(
                S(actor, 2), S(actor, 6), fx, fz);
            B(actor, 0x3B0) = 0xFF;
        } else {
            if (((D_800F3566 / 128) != 0 && (D_800F3566 / 16) >= 3) ||
                ((D_800F3566 / 128) == 0 && (D_800F3566 / 16) != 0)) {
                B(actor, 0x3B0) = 0xFF;
            } else {
                B(actor, 0x3B0) = (unsigned char)((func_800AD678() % 16) + 3);
            }
            func_800807F0(actor);
        }
        B(actor, 0x397)++;
        break;

    case 0xC1:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 5) {
            if (B(actor, 0x3B0) != 0) {
                B(actor, 0x3B0)--;
            } else {
                func_8005BA30(actor, 0x3B, 0);
                B(actor, 0x397)++;
                B(actor, 0x3B0) = (unsigned char)((func_800AD678() % 6) + 3);
            }
        }
        func_800807F0(actor);
        break;

    case 0xC2:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0x1D) {
            func_8005BA30(actor, 0x3B, 0xC);
            if (S(focusActor, 0x3C8) != 0) {
                B(actor, 0x3B0) = 0xFF;
                B(actor, 0x397)++;
            } else if (B(actor, 0x3B0) != 0) {
                B(actor, 0x3B0)--;
            } else {
                B(actor, 0x397)++;
                B(actor, 0x3B0) = (unsigned char)((func_800AD678() % 5) + 6);
            }
        }
        func_800807F0(actor);
        break;

    case 0xC3:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0x29) {
            func_8005BA30(actor, 2, 0x10);
            B(actor, 0x397) = 0xC1;
        }
        func_800807F0(actor);
        break;

    case 0xD0:
        B(actor, 0x3B0) = (unsigned char)func_8001E6B8(4);
        B(actor, 0x3AB) = (unsigned char)func_8005B36C(
            S(actor, 2), S(actor, 6), 0, -0x3B66); /* -60.824 m */
        B(actor, 0x396) = 0x2E;
        B(actor, 0x397) = 0x10;
        break;

    case 0xD1:
        W(actor, 0x3A0) = 8;
        B(actor, 0x396) = 0x2E;
        B(actor, 0x397) = 0x11;
        break;

    case 0xD8:
        func_8005BD1C(actor);
        if (B(actor, 0x3AA) == B(actor, 0x3AB)) {
            B(actor, 0x397)++;
        }
        func_800807B0(actor, 8);
        break;

    case 0xD9:
        func_8005BD1C(actor);
        SW(actor, 8) = func_8001E694(B(actor, 0x3A4), 12);
        SW(actor, 0x10) = func_8001E5A4(B(actor, 0x3A4), 12);
        H(actor, 2) = (unsigned short)(H(actor, 2) + (unsigned int)SW(actor, 8));
        H(actor, 6) = (unsigned short)(H(actor, 6) + (unsigned int)SW(actor, 0x10));
        break;

    default:
        break;
    }

    func_8005B908(actor);
}
