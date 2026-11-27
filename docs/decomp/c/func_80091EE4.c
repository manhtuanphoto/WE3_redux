/*
 * func_80091EE4
 * Purpose: Updates a goalkeeper diving/parrying catch-or-deflect action across
 *          substates (+0x397 = 0: dive setup and early-abort check; 1: mid-dive
 *          animation/velocity/ball-touch update), then applies ball deflection
 *          if contacted via func_8009462C.
 * Arguments:
 *   - actor (unsigned char *): pointer to 1000-byte player actor struct
 * Return value: void
 * Actor fields read/written:
 *   - +0x02 (s16): X position (250 raw = 1 m)
 *   - +0x04 (s16): Y position / height (negative = up)
 *   - +0x06 (s16): Z position (250 raw = 1 m)
 *   - +0x08 (s32): X velocity (raw/frame)
 *   - +0x0C (s32): Y velocity (raw/frame)
 *   - +0x10 (s32): Z velocity (raw/frame)
 *   - +0x26 (s16): model rotation ((0x1C0 - bodyHeading) % 256) * 16
 *   - +0x38D (u8): actor ID
 *   - +0x390 (u8): current animation frame
 *   - +0x392 (u8): team slot (0..10)
 *   - +0x396 (u8): actor state (set to 0x96, 0x84, or 0x83 on transitions)
 *   - +0x397 (u8): substate (0..1, cleared to 0 on state exit)
 *   - +0x398 (u8): team side (0/1)
 *   - +0x3A0 (u32): movement speed (threshold 0x30 = 48 raw/frame)
 *   - +0x3A4 (u8): move heading
 *   - +0x3AA (u8): body heading
 *   - +0x3AB (u8): target heading
 *   - +0x3AC (u8): mirror/side flag (0/1)
 *   - +0x3B0 (u8): cleared to 0 in substate 0
 *   - +0x3B9 (u8): stat value divided by 10 for reaction-time threshold
 *   - +0x3C2/+0x3C4/+0x3C6 (s16): dive target X/Y/Z
 *   - +0x3D0 (u8): early reaction/abort check flag in substate 0
 * Scratchpad fields read/written:
 *   - 0x1F8000F0/0x0F2/0x0F4 (s16): scratch X/Y/Z anchor position
 *   - 0x1F8002A8 (u8): last-touch actor ID
 *   - 0x1F8002B0 (u32): ball event code (4 = deflect/save power boost)
 *   - 0x1F8002F4 (ptr): pointer to ball actor (+0x3A4 move heading)
 *   - 0x1F80031C (s16): ball timer (frames since last touch, 30 frames/s)
 * External data read:
 *   - D_8010C32F: per-player stat byte table (stride 12 bytes/slot, 264 bytes/side;
 *                 low nibble >> 1 caps dive speed)
 * Magic numbers:
 *   - 20 - stat/10 frames (max 0.67 s at 30 Hz): reaction window in substate 0
 *   - 0x40 (64/256 = 90 deg): max heading deviation from ball flight before abort
 *   - 0x2C (44) / 0x26 (38) raw/frame (~5.28 / 4.56 m/s at 30 Hz): base dive speed caps
 *   - 0x50 (80 raw = 0.32 m): ball touch test radius
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCHPAD ((unsigned char *)0x1F800000)

extern unsigned char D_8010C32F[];

extern void func_8006EC5C(unsigned char *);
extern unsigned int func_8005B36C(int, int, int, int);
extern void func_8005C2AC(unsigned char *, unsigned int, unsigned int, unsigned int, unsigned int);
extern unsigned int func_800B1128(int);
extern int func_8001E694(unsigned int, int);
extern int func_8001E5A4(unsigned int, int);
extern void func_8005BA30(unsigned char *, unsigned int, unsigned int);
extern unsigned int func_8005B7CC(unsigned int, unsigned int, unsigned int);
extern void func_8005BD1C(unsigned char *);
extern int func_80094EEC(unsigned char *, unsigned int);
extern void func_80095E8C(unsigned char *);
extern void func_800954E8(unsigned char *);
extern int func_8001E6B8(int);
extern int func_80095F40(void);
extern void func_8009462C(unsigned char *, unsigned int, unsigned int, int, int);

void func_80091EE4(unsigned char *actor)
{
    unsigned char *scratch = SCRATCHPAD;
    unsigned char substate = B(actor, 0x397);

    if (substate == 0) {
        int reactThreshold;
        int dx, dz;
        unsigned int dist;
        unsigned int speed;
        unsigned int speedCap;
        unsigned int row;
        int statBonus;
        unsigned int targetAngle;
        unsigned int heading;
        int turn;

        reactThreshold = 0x14 - ((B(actor, 0x3B9) / 10U) & 0xFF);
        if (S(scratch, 0x31C) < reactThreshold) {
            if (B(actor, 0x3D0) != 0) {
                unsigned int aimAngle;
                unsigned char *ball;
                unsigned char ballHeading;
                unsigned int diff;

                func_8006EC5C(actor);
                if (S(scratch, 0x31C) != 1) {
                    return;
                }
                aimAngle = func_8005B36C(S(actor, 0x2), S(actor, 0x6),
                                         S(actor, 0x3C2), S(actor, 0x3C6));
                ball = *(unsigned char **)(scratch + 0x2F4);
                ballHeading = B(ball, 0x3A4);
                diff = (aimAngle - ballHeading) & 0xFF;
                if (diff < 0x81U) {
                    if ((int)diff >= 0x40) {
                        B(actor, 0x396) = 0x96;
                        B(actor, 0x397) = 0;
                        return;
                    }
                } else {
                    if (((ballHeading - aimAngle) & 0xFF) >= 0x40) {
                        B(actor, 0x396) = 0x96;
                        B(actor, 0x397) = 0;
                        return;
                    }
                }
            }
        }

        func_8005C2AC(actor, 0x62, 9, B(actor, 0x3AB), B(actor, 0x3AC));

        dx = S(actor, 0x3C2) - S(scratch, 0x0F0);
        dz = S(actor, 0x3C6) - S(scratch, 0x0F4);
        dist = func_800B1128(dx * dx + dz * dz);
        speed = dist / 10U;

        row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        statBonus = (signed int)(B(D_8010C32F, row) & 0xF) >> 1;
        if (W(actor, 0x3A0) < 0x30U) {
            speedCap = (unsigned int)(statBonus + 0x26);
        } else {
            speedCap = (unsigned int)(statBonus + 0x2C);
        }
        if (speedCap < speed) {
            speed = speedCap;
        }

        targetAngle = func_8005B36C(S(scratch, 0x0F0), S(scratch, 0x0F4),
                                    S(actor, 0x3C2), S(actor, 0x3C6));
        B(actor, 0x3A4) = (unsigned char)targetAngle;
        targetAngle &= 0xFF;

        SW(actor, 0x8) = func_8001E694(targetAngle, (int)speed);
        SW(actor, 0xC) = (S(actor, 0x3C4) - S(scratch, 0x0F2)) / 6;
        SW(actor, 0x10) = func_8001E5A4(targetAngle, (int)speed);

        func_8005BA30(actor, 0x62, 0);
        B(actor, 0x3B0) = 0;
        S(actor, 0x4) = 0;
        S(actor, 0x2) += SW(actor, 0x8);
        S(actor, 0x6) += SW(actor, 0x10);

        heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0x40);
        B(actor, 0x3AA) = (unsigned char)heading;
        turn = 0x1C0 - (int)(heading & 0xFF);
        S(actor, 0x26) = (turn % 256) * 16;
        B(actor, 0x397)++;
    } else if (substate == 1) {
        unsigned char frame;

        func_8005BD1C(actor);
        frame = B(actor, 0x390);

        if ((unsigned int)(frame - 1) < 0xFU) {
            switch (frame) {
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
                S(actor, 0x4) += SW(actor, 0xC);
                /* Fall through. */
            case 1:
            case 2:
            case 3: {
                unsigned int heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
                int turn = 0x1C0 - (int)(heading & 0xFF);
                B(actor, 0x3AA) = (unsigned char)heading;
                S(actor, 0x26) = (turn % 256) * 16;
                S(actor, 0x2) += SW(actor, 0x8);
                S(actor, 0x6) += SW(actor, 0x10);
                goto after_switch;
            }
            case 9:
                S(actor, 0x2) += SW(actor, 0x8);
                S(actor, 0x4) += SW(actor, 0xC);
                S(actor, 0x6) += SW(actor, 0x10);
                if (func_80094EEC(actor, 0x50) != 0) {
                    func_80095E8C(actor);
                }
                goto after_switch;
            case 10:
            case 11:
            case 12:
            case 13:
            case 14: {
                int vx, vz;
                if (func_80094EEC(actor, 0x50) != 0) {
                    func_80095E8C(actor);
                }
                vx = SW(actor, 0x8);
                vz = SW(actor, 0x10);
                S(actor, 0x2) += vx;
                S(actor, 0x6) += vz;
                S(actor, 0x4) -= SW(actor, 0xC);
                SW(actor, 0x8) = (94 * vx) / 100;
                SW(actor, 0x10) = (94 * vz) / 100;
                goto after_switch;
            }
            case 15:
                SW(actor, 0x8) = (7 * SW(actor, 0x8)) / 10;
                SW(actor, 0x10) = (7 * SW(actor, 0x10)) / 10;
                /* Fall through to landing deceleration. */
                break;
            }
        }

        /* Default / case 15 landing phase */
        S(actor, 0x4) = 0;
        S(actor, 0x2) += SW(actor, 0x8);
        S(actor, 0x6) += SW(actor, 0x10);
        SW(actor, 0x8) = (88 * SW(actor, 0x8)) / 100;
        SW(actor, 0x10) = (88 * SW(actor, 0x10)) / 100;

        if (B(actor, 0x390) == 0x16 && B(actor, 0x38D) == B(scratch, 0x2A8)) {
            B(actor, 0x396) = 0x84;
            B(actor, 0x397) = 0;
        } else if (B(actor, 0x390) == 0x1B) {
            B(actor, 0x396) = 0x83;
            B(actor, 0x397) = 0;
        }
    }

after_switch:
    func_800954E8(actor);
    {
        int randOffset = func_8001E6B8(0x24) + 0x20;
        unsigned int deflectAngle;
        unsigned int power;
        int lift;
        int liftParam;

        if (B(actor, 0x3AC) != 0) {
            deflectAngle = B(actor, 0x3AB) + randOffset;
        } else {
            deflectAngle = B(actor, 0x3AB) - randOffset;
        }

        if (W(scratch, 0x2B0) == 4) {
            power = 0xC;
        } else {
            power = 0xA;
        }

        lift = func_8001E6B8(0x10) + 8;
        liftParam = func_80095F40();
        func_8009462C(actor, power, deflectAngle & 0xFF, lift, liftParam);
    }
}
