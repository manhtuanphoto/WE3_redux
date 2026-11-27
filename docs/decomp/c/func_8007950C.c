/*
 * func_8007950C
 * Purpose: Updates a wall defender during a free-kick set piece across substates
 *          (+0x397 = 0..6: idle wall wait, kicker approach check, jump-timing
 *          countdown, airborne wall-jump with rotation/height oscillation,
 *          post-kick recovery delay, and return to state 0), and tests for
 *          ball deflection against the wall via func_8009462C.
 * Arguments:
 *   - actor (unsigned char *): pointer to 1000-byte wall defender actor struct
 * Return value: void
 * Actor fields read/written:
 *   - +0x02 (s16): X position (250 raw = 1 m)
 *   - +0x04 (s16): Y position / height (negative = up)
 *   - +0x06 (s16): Z position (250 raw = 1 m)
 *   - +0x26 (s16): model rotation ((0x1C0 - bodyHeading) % 256) * 16
 *   - +0x38D (u8): actor ID (even IDs may skip jumping when rand(3)==0)
 *   - +0x390 (u8): current animation frame
 *   - +0x396 (u8): actor state (cleared to 0 on exit)
 *   - +0x397 (u8): substate (0..6)
 *   - +0x399 (u8): attack direction flag (0/1)
 *   - +0x3A0 (u32): movement speed (cleared to 0 in substate 0)
 *   - +0x3AA (u8): body heading
 *   - +0x3AB (u8): target heading (initialized to body heading in substate 0)
 *   - +0x3AC (u8): random wall-lean sign/timer value (0..39)
 *   - +0x3B0 (u8): countdown timer (frames)
 *   - +0x3C6/+0x3C8 (s16): anchored wall X/Z position (saved in 0, restored in 1)
 *   - +0x3CA (s16): vertical jump step (-4 or -3)
 *   - +0x3CC (s16): angular spin step per frame during jump (0..7)
 *   - +0x3D1 (u8): cleared to 0 in substate 0
 * Scratchpad fields read:
 *   - 0x1F800036 (s16): formation/marking target X
 *   - 0x1F800066 (s16): formation/marking target Z
 *   - 0x1F80029A (u8): match phase (1 = open play / kick taken)
 *   - 0x1F8002AB (u8): previous ball owner ID (mod 24)
 *   - 0x1F8002AD (u8): set-piece kicker actor index (mod 24)
 *   - 0x1F8002F4 (ptr): pointer to ball actor (+0x02 X, +0x06 Z, +0x3A4 move heading)
 * External data read:
 *   - D_800FF748: base of 24-element 1000-byte actor array
 * Magic numbers:
 *   - 0x3FFFF (262143 -> dist <= 512 raw = 2.048 m): kicker-to-ball trigger distance
 *   - 0x2A3FFFF (~6656 raw = 26.6 m)^2 / 0x3FFFFFF (~8192 raw = 32.8 m)^2: shot distance tiers
 *   - 0x3C (60/256 = 84.4 deg): max angle delta clamp for wall deflection
 *   - 0x3320 (13088 raw = 52.352 m): goal-line X coordinate
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCHPAD ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];

extern void func_8005BA30(unsigned char *, unsigned int, unsigned int);
extern int func_8001E6B8(int);
extern void func_8005BD1C(unsigned char *);
extern unsigned int func_8005B36C(int, int, int, int);
extern unsigned int func_8005B7CC(unsigned int, unsigned int, unsigned int);
extern int func_8004E5A8(unsigned int);
extern unsigned int func_8009462C(unsigned char *, unsigned int, unsigned int, int, int);
extern void func_80059E4C(void);

void func_8007950C(unsigned char *actor)
{
    unsigned char *scratch = SCRATCHPAD;
    unsigned char substate = B(actor, 0x397);

    if (substate < 7) {
        switch (substate) {
        case 0: {
            int r;
            func_8005BA30(actor, 0x49, 0);
            B(actor, 0x3D1) = 0;
            W(actor, 0x3A0) = 0;
            B(actor, 0x3AB) = B(actor, 0x3AA);
            func_8001E6B8(2);
            r = func_8001E6B8(0x28);
            B(actor, 0x3AC) = (unsigned char)r;
            B(actor, 0x3B0) = (unsigned char)(r + 0x10);
            S(actor, 0x3C6) = S(actor, 0x2);
            S(actor, 0x3C8) = S(actor, 0x6);
            B(actor, 0x397)++;
            return;
        }
        case 1: {
            unsigned char timer = B(actor, 0x3B0);
            unsigned char *kicker;
            unsigned char *ball;
            int dx, dz;

            if (timer != 0) {
                B(actor, 0x3B0) = timer - 1;
            } else {
                func_8005BD1C(actor);
                if (B(actor, 0x390) == 0xF) {
                    int r;
                    func_8005BA30(actor, 0x49, 0);
                    func_8001E6B8(2);
                    r = func_8001E6B8(0x28);
                    B(actor, 0x3AC) = (unsigned char)r;
                    B(actor, 0x3B0) = (unsigned char)(r + 0x10);
                }
            }

            kicker = D_800FF748 + (B(scratch, 0x2AD) % 24U) * 1000;
            ball = *(unsigned char **)(scratch + 0x2F4);
            dx = S(kicker, 0x2) - S(ball, 0x2);
            dz = S(kicker, 0x6) - S(ball, 0x6);

            if (dx * dx + dz * dz <= 0x3FFFF) {
                if (func_8001E6B8(3) == 0 && (B(actor, 0x38D) & 1) == 0) {
                    func_8005BA30(actor, 0x49, 0);
                    B(actor, 0x397) = 4;
                } else {
                    unsigned char *ball2;
                    int bx, gx;
                    unsigned int distSq;
                    int delay;

                    func_8005BA30(actor, 0x4A, 0);
                    ball2 = *(unsigned char **)(scratch + 0x2F4);
                    bx = S(ball2, 0x2);
                    gx = (B(kicker, 0x399) != 0) ? (bx + 0x3320) : (bx - 0x3320);
                    distSq = (unsigned int)(gx * gx + S(ball2, 0x6) * S(ball2, 0x6));

                    if (distSq > 0x2A3FFFFU) {
                        if (distSq > 0x3FFFFFFU) {
                            if (S(kicker, 0x3C8) != 0) {
                                delay = func_8001E6B8(2) + 1;
                            } else {
                                delay = func_8001E6B8(4) + 4;
                            }
                        } else {
                            delay = func_8001E6B8(4) + 3;
                        }
                    } else {
                        delay = func_8001E6B8(4) + 4;
                    }
                    B(actor, 0x3B0) = (unsigned char)delay;
                    B(actor, 0x397)++;
                }
            }

            S(actor, 0x2) = S(actor, 0x3C6);
            S(actor, 0x6) = S(actor, 0x3C8);
            return;
        }
        case 2: {
            unsigned char rem = B(actor, 0x3B0) - 1;
            B(actor, 0x3B0) = rem;
            if (rem == 0) {
                S(actor, 0x3CA) = (signed short)(func_8001E6B8(2) - 4);
                if (func_8001E6B8(2) != 0) {
                    S(actor, 0x3CC) = (signed short)func_8001E6B8(8);
                } else {
                    S(actor, 0x3CC) = 0;
                }
                B(actor, 0x397)++;
            }
            break;
        }
        case 3: {
            unsigned char frame;
            func_8005BD1C(actor);
            frame = B(actor, 0x390);
            if ((unsigned int)(frame - 0xA) < 0x15U) {
                switch (frame) {
                case 10:
                case 11:
                case 12:
                case 13:
                case 14:
                case 15: {
                    int spin = S(actor, 0x3CC);
                    unsigned char h = B(actor, 0x3AA);
                    int turn;
                    if (B(actor, 0x3AC) != 0) {
                        h = (unsigned char)(h - spin);
                    } else {
                        h = (unsigned char)(h + spin);
                    }
                    B(actor, 0x3AA) = h;
                    turn = 0x1C0 - (int)h;
                    S(actor, 0x26) = (turn % 256) * 16;
                    S(actor, 0x4) -= S(actor, 0x3CA);
                    break;
                }
                case 16:
                case 17:
                case 18:
                case 19:
                case 20: {
                    int spin = S(actor, 0x3CC);
                    unsigned char h = B(actor, 0x3AA);
                    int turn;
                    if (B(actor, 0x3AC) != 0) {
                        h = (unsigned char)(h - spin);
                    } else {
                        h = (unsigned char)(h + spin);
                    }
                    B(actor, 0x3AA) = h;
                    turn = 0x1C0 - (int)h;
                    S(actor, 0x26) = (turn % 256) * 16;
                    S(actor, 0x4) += S(actor, 0x3CA);
                    break;
                }
                case 21:
                    S(actor, 0x4) = 0;
                    break;
                case 30:
                    B(actor, 0x396) = 0;
                    B(actor, 0x397) = 0;
                    break;
                }
            }
            break;
        }
        case 4:
            if (B(scratch, 0x29A) == 1) {
                B(actor, 0x3B0) = (unsigned char)(func_8001E6B8(0xC) + 0x10);
                B(actor, 0x397)++;
            }
            break;
        case 5: {
            unsigned char rem = B(actor, 0x3B0) - 1;
            B(actor, 0x3B0) = rem;
            if (rem == 0) {
                B(actor, 0x397)++;
            }
            break;
        }
        case 6:
            func_8005BD1C(actor);
            B(actor, 0x390) = 0xE;
            B(actor, 0x396) = 0;
            B(actor, 0x397) = 0;
            break;
        }
    }

    {
        unsigned char *ball = *(unsigned char **)(scratch + 0x2F4);
        int ax = S(actor, 0x2);
        int az = S(actor, 0x6);
        int bx = S(ball, 0x2);
        int bz = S(ball, 0x6);
        int mx = S(scratch, 0x036);
        int mz = S(scratch, 0x066);
        int dBall = (ax - bx) * (ax - bx) + (az - bz) * (az - bz);
        int dMark = (ax - mx) * (ax - mx) + (az - mz) * (az - mz);
        unsigned int revBallHeading = (unsigned int)(B(ball, 0x3A4) - 0x80);
        int tx = (dBall < dMark) ? bx : mx;
        int tz = (dBall < dMark) ? bz : mz;
        unsigned int aimAngle = func_8005B36C(ax, az, tx, tz);
        unsigned int diff = (aimAngle - revBallHeading) & 0xFF;
        unsigned int angleDelta;
        int jitter;
        unsigned int ballH;
        unsigned int targetAngle;
        unsigned int turnStep;
        unsigned int deflectAngle;
        int liftParam;

        if (diff < 0x81U) {
            angleDelta = diff;
        } else {
            angleDelta = (revBallHeading - aimAngle) & 0xFF;
        }
        if ((angleDelta & 0xFF) >= 0x3DU) {
            angleDelta = 0x3C;
        } else {
            angleDelta &= 0xFF;
        }

        jitter = func_8001E6B8(3) / 10;
        ballH = B(*(unsigned char **)(scratch + 0x2F4), 0x3A4);
        if (((aimAngle - ballH - 0x80U) & 0xFF) < 0x81U) {
            targetAngle = (ballH - 0x10U) & 0xFF;
        } else {
            targetAngle = (ballH + 0x10U) & 0xFF;
        }

        turnStep = (0x80U - (angleDelta * 11U + (unsigned int)jitter)) & 0xFF;
        deflectAngle = func_8005B7CC(revBallHeading & 0xFF, targetAngle, turnStep) & 0xFF;
        liftParam = func_8004E5A8(deflectAngle);

        if ((func_8009462C(actor, 0xA, deflectAngle, 0, liftParam) & 0xFF) != 0) {
            unsigned char *prevActor = D_800FF748 + (B(scratch, 0x2AB) % 24U) * 1000;
            if (B(actor, 0x399) != B(prevActor, 0x399)) {
                func_80059E4C();
            }
        }
    }
}
