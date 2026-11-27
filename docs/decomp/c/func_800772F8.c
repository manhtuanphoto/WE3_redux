/*
 * func_800772F8
 *
 * Purpose:
 *   Two-step state machine (D_8010C538 == 0 -> 1) that initializes and sets up
 *   a free kick after a foul: checks for red-card dismissal, places and clamps
 *   the ball position (including randomized positions in mode 5 / practice),
 *   transitions all players into free-kick setup state (0x30), selects and
 *   positions the free-kick taker 6.144 m behind the ball, sets up the wall/UI
 *   if in the attacking half, and clears match-stoppage state.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   void
 *
 * Scratchpad (0x1F800000) reads/writes:
 *   - 0x1E8 (u8, W): cleared to 0 in step 1
 *   - 0x1EC (u8, W): cleared to 0 in step 0
 *   - 0x294 (s16, W): cleared to 0 in step 1
 *   - 0x296 (s16, W): cleared to 0 in step 1
 *   - 0x2A8 (u8, W): last-touch actor id cleared to 0 in step 1
 *   - 0x2AD (u8, R/W): offending / reference actor id (set to 0xFF at end of step 0)
 *   - 0x2AE (u8, R/W): fouled / free-kick reference actor id (updated to kicker in step 1)
 *   - 0x2B0 (u32, W): ball event code cleared to 0 in step 0
 *   - 0x2B8 (u16, W): set to 0x7FFF sentinel in step 1
 *   - 0x2BC/0x2BE (s16, R/W): foul spot X/Z coordinates (clamped out of goal area in step 0)
 *   - 0x2CF (u8[2], W): possession flags (offender's side = 0, opponent = 1)
 *   - 0x2E1 (u8, R): mode (5 = training/free-kick practice with randomized ball spot)
 *   - 0x2F2 (u8, W): camera tracking mode cleared to 0 in step 1
 *   - 0x2F4 (u32, R): pointer to ball actor
 *   - 0x309 (u8, W): set to 1 in step 1
 *   - 0x31E/0x31F (u8[2], W): primary/secondary actor ids indexed by attackDir * 2
 *   - 0x334 (u16, R): flag triggering func_800729CC if nonzero
 *
 * Actor fields read/written:
 *   - +0x00 (u8, W): active flag (cleared to 0 on offender if card count >= 2)
 *   - +0x02 (s16, R/W): X position (ball & kicker)
 *   - +0x04 (s16, W): Y position (-0x20 = resting on turf)
 *   - +0x06 (s16, R/W): Z position (ball & kicker)
 *   - +0x38D (u8, R): actor id
 *   - +0x38E (u16, R): current anim id
 *   - +0x392 (u8, R): team slot (0..10)
 *   - +0x394 (u16, R): actor role/stat field (checked == 0x297 or 0x373)
 *   - +0x396 (u8, R/W): state (ball = 0; active non-0x35/0xFF players = 0x30)
 *   - +0x397 (u8, W): substate (players = 0, focus = 1, kicker = 0xA)
 *   - +0x398 (u8, R): side (0/1)
 *   - +0x399 (u8, R): attack direction flag (0/1)
 *   - +0x3A0 (u32, W): speed cleared to 0 on kicker
 *   - +0x3A4 (u8, W): ball move heading set to goal direction
 *   - +0x3AA (u8, W): kicker body heading toward ball
 *   - +0x3AB (u8, W): focus actor target heading toward goal
 *   - +0x3AC (u8, W): mirror/foot flag from D_8010C330
 *   - +0x3C8 (s16, W): status flag (1 if +0x394 == 0x297 or 0x373, else 0)
 *   - +0x3E2/+0x3E3 (u8, W): set to 2 and 0 on reset players
 *
 * Globals read/written:
 *   - D_8010C538 (u8, R/W): free-kick setup sub-phase (0 -> 1 -> 0)
 *   - D_800FF748 (u8[]): 24 * 1000-byte actor pool (plus team/stat tables at +0x59D8, +0x64A4, +0x88A5, +0x8ED4)
 *   - D_8010A5BC (u8, W): offender's card count from D_800FF748 + 0x88A5
 *   - D_8010C330 (u32[]): player info table (12 bytes/slot, 264 bytes/side; bit 4 -> +0x3AC)
 *   - D_8010A327 (u8, R), D_8010A5A8 (u8[], R): match/UI flags
 *   - D_8010591C/D_80105944/D_8010596C/D_80105C8C/D_80105B9C/D_80105BC4/D_80107FEC (u8, W): cleared to 0
 *
 * Magic numbers / units (250 raw = 1 m, 30 frames/s):
 *   - -0x20 (-32) = -0.128 m (ball resting height above pitch)
 *   - 0x12F4 (4852) = 19.408 m (penalty area half-width Z)
 *   - 0x23E2 (9186) = 36.744 m, 0xCC8 (3272) = 13.088 m (random FK X bounds)
 *   - 0x8B6 (2230) = 8.92 m, 0x2E08 (11784) = 47.136 m (goal-area Z/X exclusion bounds)
 *   - 0x32E0 (13024) = 52.096 m (goal-line X clamp), 0x20C0 (8384) = 33.536 m (touchline Z clamp)
 *   - 0x3320 (13088) = 52.352 m (goal center X)
 *   - 0x600 (1536) = 6.144 m (kicker stand-off distance from ball)
 *   - 0x2080 (8320) = 33.28 m (kicker Z clamp), -0x23E1 / +-0x22E2 (+-8930 = +-35.72 m) (kicker X clamp)
 *   - 0x20 (32) = 45 deg run-up angle offset
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010591C;
extern unsigned char D_80105944;
extern unsigned char D_8010596C;
extern unsigned char D_80105B9C;
extern unsigned char D_80105BC4;
extern unsigned char D_80105C8C;
extern unsigned char D_80107FEC;
extern unsigned char D_8010A327;
extern unsigned char D_8010A5A8[];
extern unsigned char D_8010A5BC;
extern unsigned char D_8010C330[];
extern unsigned char D_8010C538;

extern int func_8004FBC4(int);
extern void func_80098428(void);
extern void func_80089BD8(void);
extern void func_800732B0(void);
extern void func_800729CC(void);
extern void func_8004ED28(int);
extern void func_8001D470(int, int, int, int, int, int, int, int, int);
extern int func_8001E6B8(int);
extern int func_800AD678(void);
extern void func_800A69D8(void);
extern void func_8005BA30(unsigned char *, int, int);
extern int func_8005B36C(int, int, int, int);
extern void func_80078CE4(unsigned char *, int);
extern void func_800598C8(unsigned char *);
extern void func_800676C4(int, int, int, int);
extern unsigned char func_80049D94(unsigned char *);
extern int func_8001E694(int, int);
extern int func_8001E5A4(int, int);
extern void func_8005B864(unsigned char *, int);
extern void func_800512B8(unsigned char *, int);
extern void func_800204D8(void);
extern void func_8001CEE0(int, int, int, int, int, int, int, int);
extern void func_80079C50(unsigned char *);
extern void func_800171DC(void);

void func_800772F8(void)
{
    unsigned char step = D_8010C538;

    if (step == 0) {
        unsigned char *actor;
        unsigned char *ball;
        unsigned char cardCount;
        signed short bx;
        signed short bz;

        if (func_8004FBC4(0) == 0) {
            return;
        }
        func_80098428();
        func_80089BD8();

        B(Scratchpad, 0x1EC) = 0;
        D_8010591C = 0;
        D_80105944 = 0;
        D_8010596C = 0;
        B(D_800FF748, 0x59D8) = 0;

        actor = D_800FF748 + (B(Scratchpad, 0x2AD) % 24) * 1000;
        cardCount = B(D_800FF748 + 0x88A5 + B(actor, 0x398) * 22 + B(actor, 0x392), 0);
        D_8010A5BC = cardCount;
        if (cardCount >= 2) {
            /* Deactivate dismissed player */
            B(actor, 0x00) = 0;
            func_800732B0();
        }

        if (H(Scratchpad, 0x334) != 0) {
            func_800729CC();
        }

        actor = D_800FF748 + (B(Scratchpad, 0x2AE) % 24) * 1000;
        B(D_800FF748 + 0x8ED4 + B(actor, 0x398), 0) += 1;
        func_8004ED28(8);

        if (B(Scratchpad, 0x2E1) == 5) {
            func_8001D470(0x11, 9, 0, 0, 0x40, 0x1000, 0x1000, 0x1000, 0);
            D_80105C8C = 0;
            D_80105B9C = 0;
            D_80105BC4 = 0;

            if (D_8010A327 & 1) {
                int r;
                int sign;
                signed short absZ;

                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                S(ball, 0x04) = -0x20;

                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                r = func_8001E6B8(20);
                sign = func_800AD678();
                if (sign & 1) {
                    S(ball, 0x06) = (signed short)((r * 8448) / 20);
                } else {
                    S(ball, 0x06) = (signed short)((-(r * 8448)) / 20);
                }

                absZ = S(((unsigned char *)W(Scratchpad, 0x2F4)), 0x06);
                if (absZ < 0) {
                    absZ = -absZ;
                }
                if (absZ < 0x12F4) {
                    int rx = func_8001E6B8(10);
                    S(((unsigned char *)W(Scratchpad, 0x2F4)), 0x02) = (signed short)(0x23E2 - (rx * 4593) / 10);
                } else {
                    int rx = func_8001E6B8(20);
                    S(((unsigned char *)W(Scratchpad, 0x2F4)), 0x02) = (signed short)(0xCC8 + (rx * 9816) / 20);
                }

                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                if (S(ball, 0x02) >= 0x23E3) {
                    signed short zVal = S(ball, 0x06);
                    signed short absZ2 = (zVal < 0) ? -zVal : zVal;
                    if (absZ2 < 0x12F4) {
                        int rz = func_8001E6B8(10);
                        int off = (rz * 3596) / 10;
                        if (zVal != 0) {
                            S(ball, 0x06) = (signed short)(0x12F4 + off);
                        } else {
                            S(ball, 0x06) = (signed short)(-0x12F4 - off);
                        }
                    }
                }
            }
        } else {
            signed short foulZ = S(Scratchpad, 0x2BE);
            signed short absFoulZ = (foulZ < 0) ? -foulZ : foulZ;

            if (absFoulZ < 0x8B6) {
                if (B(actor, 0x399) == 0) {
                    if (S(Scratchpad, 0x2BC) < -0x2E07) {
                        S(Scratchpad, 0x2BC) = -0x2E08;
                        S(Scratchpad, 0x2BE) = (foulZ > 0) ? 0x8B6 : -0x8B6;
                    }
                }
                if (B(actor, 0x399) != 0) {
                    if (S(Scratchpad, 0x2BC) >= 0x2E08) {
                        S(Scratchpad, 0x2BC) = 0x2E08;
                        S(Scratchpad, 0x2BE) = (S(Scratchpad, 0x2BE) > 0) ? 0x8B6 : -0x8B6;
                    }
                }
            }

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            S(ball, 0x02) = S(Scratchpad, 0x2BC);
            S(ball, 0x04) = -0x20;
            S(ball, 0x06) = S(Scratchpad, 0x2BE);
        }

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        bx = S(ball, 0x02);
        if (bx >= 0x32E1) {
            S(ball, 0x02) = 0x32E0;
        } else if (bx < -0x32E0) {
            S(ball, 0x02) = -0x32E0;
        }

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        bz = S(ball, 0x06);
        if (bz >= 0x20C1) {
            S(ball, 0x06) = 0x20C0;
        } else if (bz < -0x20C0) {
            S(ball, 0x06) = -0x20C0;
        }

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        B(ball, 0x396) = 0;
        W(Scratchpad, 0x2B0) = 0;
        B(Scratchpad, 0x2CF + B(actor, 0x398)) = 0;
        B(Scratchpad, 0x2CF + (B(actor, 0x398) ^ 1)) = 1;
        B(Scratchpad, 0x2AD) = 0xFF;
        D_8010C538 += 1;
    } else if (step == 1) {
        int i;
        unsigned char *player;
        unsigned char *focusActor;
        unsigned char *kicker;
        unsigned char *ball;
        int goalAngle;
        int offsetAngle;
        int goalX;
        unsigned int row;
        unsigned char kickerId;
        signed short bx;
        signed short kz;
        signed short kx;
        unsigned char atk;

        D_80107FEC = 0;
        func_800A69D8();

        player = D_800FF748 + 0x3E8;
        for (i = 0; i < 22; i++, player += 1000) {
            unsigned char st = B(player, 0x396);
            if (st != 0x35 && st != 0xFF) {
                if (H(player, 0x38E) != 1) {
                    int frame = func_8001E6B8(16) & 0xFF;
                    func_8005BA30(player, 1, frame);
                }
                B(player, 0x396) = 0x30;
                B(player, 0x397) = 0;
                B(player, 0x3E2) = 2;
                B(player, 0x3E3) = 0;
            }
        }

        focusActor = D_800FF748 + (B(Scratchpad, 0x2AD) % 24) * 1000;
        if (H(focusActor, 0x394) == 0x297 || H(focusActor, 0x394) == 0x373) {
            S(focusActor, 0x3C8) = 1;
        } else {
            S(focusActor, 0x3C8) = 0;
        }

        B(Scratchpad, 0x2A8) = 0;
        func_8005BA30(focusActor, 0x2F, 0);
        B(focusActor, 0x397) = 1;

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        goalX = (B(focusActor, 0x399) != 0) ? -0x3320 : 0x3320;
        goalAngle = func_8005B36C(S(ball, 0x02), S(ball, 0x06), goalX, 0);

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        B(ball, 0x3A4) = (unsigned char)goalAngle;
        B(focusActor, 0x3AB) = (unsigned char)goalAngle;

        row = (unsigned int)B(focusActor, 0x392) * 12 + (unsigned int)B(focusActor, 0x398) * 264;
        B(focusActor, 0x3AC) = (unsigned char)((W(D_8010C330 + row) >> 4) & 1);
        func_80078CE4(focusActor, goalAngle & 0xFF);
        func_800598C8(focusActor);

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        func_800676C4(B(focusActor, 0x399), S(ball, 0x02), S(ball, 0x06), 0);

        kickerId = func_80049D94(focusActor);
        kicker = D_800FF748 + (kickerId % 24) * 1000;

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        atk = B(focusActor, 0x399);
        bx = S(ball, 0x02);
        if (atk != 0) {
            bx = -bx;
        }
        if (bx < 0x23E2) {
            if (((unsigned int)(goalAngle - (atk << 7)) & 0xFF) < 0x81) {
                offsetAngle = goalAngle - 0x20;
            } else {
                offsetAngle = goalAngle + 0x20;
            }
        } else {
            if (((unsigned int)(goalAngle - (atk << 7)) & 0xFF) < 0x81) {
                offsetAngle = goalAngle + 0x20;
            } else {
                offsetAngle = goalAngle - 0x20;
            }
        }
        offsetAngle &= 0xFF;

        S(kicker, 0x02) = (signed short)(S((unsigned char *)W(Scratchpad, 0x2F4), 0x02) + func_8001E694(offsetAngle, 0x600));
        S(kicker, 0x06) = (signed short)(S((unsigned char *)W(Scratchpad, 0x2F4), 0x06) + func_8001E5A4(offsetAngle, 0x600));

        kz = S(kicker, 0x06);
        if (kz >= 0x2081) {
            S(kicker, 0x06) = 0x2080;
        } else if (kz < -0x2080) {
            S(kicker, 0x06) = -0x2080;
        }

        kx = S(kicker, 0x02);
        if (B(focusActor, 0x399) != 0) {
            if (-kx < -0x23E1) {
                S(kicker, 0x02) = 0x22E2;
            }
        } else {
            if (kx < -0x23E1) {
                S(kicker, 0x02) = -0x22E2;
            }
        }

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        B(kicker, 0x3AA) = (unsigned char)func_8005B36C(S(kicker, 0x02), S(kicker, 0x06), S(ball, 0x02), S(ball, 0x06));
        W(kicker, 0x3A0) = 0;
        func_8005B864(kicker, 0);

        B(Scratchpad, 0x31E + B(focusActor, 0x399) * 2) = B(focusActor, 0x38D);
        B(Scratchpad, 0x31F + B(focusActor, 0x399) * 2) = B(kicker, 0x38D);
        func_800512B8(focusActor, 6);

        B(Scratchpad, 0x2AE) = B(kicker, 0x38D);
        B(kicker, 0x397) = 0xA;
        func_800204D8();

        B(Scratchpad, 0x2F2) = 0;
        B(Scratchpad, 0x309) = 1;
        H(Scratchpad, 0x2B8) = 0x7FFF;
        S(Scratchpad, 0x294) = 0;
        S(Scratchpad, 0x296) = 0;
        D_8010C538 = 0;
        B(Scratchpad, 0x1E8) = 0;
        B(D_800FF748, 0x64A4) = B(D_8010A5A8, 0xA) & 3;

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        bx = S(ball, 0x02);
        if (B(focusActor, 0x399) != 0 ? (bx < 0) : (bx > 0)) {
            func_8001CEE0(0x22, 0x46, 0, 0, -0x14, 6, 0x58, 1);
            func_80079C50(focusActor);
        }
        func_800171DC();
    }
}
