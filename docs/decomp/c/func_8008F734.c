/*
 * func_8008F734
 * Purpose: Updates a goalkeeper leaping/high-claim or punch-clear action across
 *          substates (+0x397 = 0: jump setup and early-abort check; 1: mid-air
 *          animation/velocity/ball-touch update), then applies ball punch/deflect
 *          via func_8009462C.
 * Arguments:
 *   - actor (unsigned char *): pointer to 1000-byte player actor struct
 * Return value: void
 * Actor fields read/written:
 *   - +0x02 (s16): X position (250 raw = 1 m)
 *   - +0x04 (s16): Y position / height (negative = up)
 *   - +0x06 (s16): Z position (250 raw = 1 m)
 *   - +0x08 (s32): X velocity (raw/frame)
 *   - +0x0C (s32): Y velocity (raw/frame, halved via >>1 on frame 6)
 *   - +0x10 (s32): Z velocity (raw/frame)
 *   - +0x26 (s16): model rotation ((0x1C0 - bodyHeading) % 256) * 16
 *   - +0x38D (u8): actor ID
 *   - +0x390 (u8): current animation frame
 *   - +0x392 (u8): team slot (0..10)
 *   - +0x396 (u8): actor state (set to 0x82, 0x84, or 0x8D on transitions)
 *   - +0x397 (u8): substate (0..1, reset to 0 or 1 on state exit)
 *   - +0x398 (u8): team side (0/1)
 *   - +0x3A0 (u32): movement speed (cleared to 0 in substate 0)
 *   - +0x3A4 (u8): move heading (set to target heading in substate 0)
 *   - +0x3AA (u8): body heading
 *   - +0x3AB (u8): target heading
 *   - +0x3AC (u8): mirror/side flag (0/1)
 *   - +0x3B0 (u8): jump variant flag (0 vs 5 vs other)
 *   - +0x3B9 (u8): stat value divided by 10 for reaction-time / timer jitter
 *   - +0x3C2/+0x3C4/+0x3C6 (s16): jump target X/Y/Z
 * Scratchpad fields read/written:
 *   - 0x1F8000F0/0x0F2/0x0F4 (s16): scratch X/Y/Z anchor position
 *   - 0x1F8002A8 (u8): last-touch actor ID
 *   - 0x1F80031C (s16): ball timer (frames since last touch, 30 frames/s)
 * External data read:
 *   - D_8010C328 / D_8010C32F: per-player stat struct (stride 12 bytes/slot, 264 bytes/side)
 * Magic numbers:
 *   - 19 - stat/10 frames (max 0.63 s at 30 Hz): reaction window in substate 0
 *   - 0x26 (38 raw/frame = 4.56 m/s at 30 Hz) + stat: horizontal leap speed cap
 *   - 0x18 (24 raw/frame): downward Y velocity clamp; -(24 + stat) upward clamp
 *   - 0xA0 (160 raw = 0.64 m): ball touch test radius
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCHPAD ((unsigned char *)0x1F800000)

extern unsigned char D_8010C328[];
extern unsigned char D_8010C32F[];

extern void func_8006EC5C(unsigned char *);
extern void func_8005BA30(unsigned char *, unsigned int, unsigned int);
extern unsigned int func_8005B7CC(unsigned int, unsigned int, unsigned int);
extern void func_8005C2AC(unsigned char *, unsigned int, unsigned int, unsigned int, unsigned int);
extern unsigned int func_800B1128(int);
extern unsigned int func_8005B36C(int, int, int, int);
extern int func_8001E694(unsigned int, int);
extern int func_8001E5A4(unsigned int, int);
extern void func_80094474(int *, int, int);
extern void func_8005BD1C(unsigned char *);
extern int func_80094EEC(unsigned char *, unsigned int);
extern int func_8001E6B8(int);
extern void func_80095E8C(unsigned char *);
extern void func_800954E8(unsigned char *);
extern int func_80095F40(void);
extern void func_8009462C(unsigned char *, unsigned int, unsigned int, int, int);

void func_8008F734(unsigned char *actor)
{
    unsigned char *scratch = SCRATCHPAD;
    unsigned char substate = B(actor, 0x397);

    if (substate == 0) {
        int reactThreshold = 0x13 - ((B(actor, 0x3B9) / 10U) & 0xFF);
        unsigned int heading;
        int turn;
        int dx, dz;
        unsigned int dist;
        unsigned int speed;
        unsigned int speedCap;
        unsigned int row;
        int statBonus;
        unsigned int targetAngle;

        if (S(scratch, 0x31C) < reactThreshold) {
            func_8006EC5C(actor);
            if (S(scratch, 0x31C) == 1) {
                B(actor, 0x396) = 0x82;
                B(actor, 0x397) = 0;
            }
            return;
        }

        func_8005BA30(actor, 0x71, B(actor, 0x3B0));
        W(actor, 0x3A0) = 0;
        heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0x40);
        B(actor, 0x3AA) = (unsigned char)heading;
        B(actor, 0x3A4) = B(actor, 0x3AB);
        turn = 0x1C0 - (int)B(actor, 0x3AA);
        S(actor, 0x26) = (turn % 256) * 16;

        func_8005C2AC(actor, 0x71, (B(actor, 0x3B0) + 6) & 0xFF, B(actor, 0x3AB), B(actor, 0x3AC));

        dx = S(actor, 0x3C2) - S(scratch, 0x0F0);
        dz = S(actor, 0x3C6) - S(scratch, 0x0F4);
        dist = func_800B1128(dx * dx + dz * dz);
        speed = dist / 7U;

        row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        statBonus = (signed int)(B(D_8010C32F, row) & 0xF) >> 1;
        speedCap = (unsigned int)(statBonus + 0x26);
        if (speedCap < speed) {
            speed = speedCap;
        }

        targetAngle = func_8005B36C(S(scratch, 0x0F0), S(scratch, 0x0F4),
                                    S(actor, 0x3C2), S(actor, 0x3C6)) & 0xFF;
        SW(actor, 0x8) = func_8001E694(targetAngle, (int)speed);
        SW(actor, 0x10) = func_8001E5A4(targetAngle, (int)speed);

        if (B(actor, 0x3B0) == 0) {
            unsigned int row2;
            int minVy;
            S(actor, 0x4) = 0;
            SW(actor, 0xC) = (S(actor, 0x3C4) - S(scratch, 0x0F2)) / 4;
            row2 = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
            minVy = -0x18 - ((signed int)(B(D_8010C32F, row2) & 0xF) >> 1);
            func_80094474(&SW(actor, 0xC), 0x18, minVy);
        } else {
            S(actor, 0x4) = (signed short)SW(actor, 0xC);
            SW(actor, 0xC) = 0;
        }

        S(actor, 0x2) += SW(actor, 0x8);
        S(actor, 0x6) += SW(actor, 0x10);
        B(actor, 0x397)++;
    } else if (substate == 1) {
        unsigned char frame;

        func_8005BD1C(actor);
        frame = B(actor, 0x390);

        if ((unsigned int)(frame - 1) < 0xDU) {
            switch (frame) {
            case 3:
            case 4:
            case 5:
                S(actor, 0x4) += SW(actor, 0xC);
                /* Fall through. */
            case 1:
            case 2: {
                unsigned int heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
                int turn = 0x1C0 - (int)(heading & 0xFF);
                B(actor, 0x3AA) = (unsigned char)heading;
                S(actor, 0x26) = (turn % 256) * 16;
                S(actor, 0x2) += SW(actor, 0x8);
                S(actor, 0x6) += SW(actor, 0x10);
                goto after_switch;
            }
            case 6:
                S(actor, 0x4) += SW(actor, 0xC);
                S(actor, 0x2) += SW(actor, 0x8);
                S(actor, 0x6) += SW(actor, 0x10);
                if (B(actor, 0x3B0) == 0 && func_80094EEC(actor, 0xA0) != 0) {
                    unsigned int row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
                    int rnd = func_8001E6B8((int)(W(D_8010C328, row) >> 28) + 2);
                    S(scratch, 0x31C) += rnd;
                    func_80095E8C(actor);
                }
                SW(actor, 0xC) = SW(actor, 0xC) >> 1;
                goto after_switch;
            case 11:
                S(actor, 0x2) += SW(actor, 0x8);
                S(actor, 0x6) += SW(actor, 0x10);
                S(actor, 0x4) -= SW(actor, 0xC);
                if (B(actor, 0x3B0) == 5 && func_80094EEC(actor, 0xA0) != 0) {
                    int rnd = func_8001E6B8((int)(B(actor, 0x3B9) / 10U) + 4);
                    S(scratch, 0x31C) += rnd;
                    func_80095E8C(actor);
                }
                goto after_switch;
            case 7:
            case 8:
            case 9: {
                unsigned int heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
                int turn = 0x1C0 - (int)(heading & 0xFF);
                B(actor, 0x3AA) = (unsigned char)heading;
                S(actor, 0x26) = (turn % 256) * 16;
                /* Fall through. */
            }
            case 10:
            case 12:
            case 13:
                S(actor, 0x2) += SW(actor, 0x8);
                S(actor, 0x4) -= SW(actor, 0xC);
                S(actor, 0x6) += SW(actor, 0x10);
                goto after_switch;
            }
        }

        /* Default landing phase */
        S(actor, 0x4) = 0;
        S(actor, 0x2) += SW(actor, 0x8);
        S(actor, 0x6) += SW(actor, 0x10);
        SW(actor, 0x8) = (94 * SW(actor, 0x8)) / 100;
        SW(actor, 0x10) = (94 * SW(actor, 0x10)) / 100;

        if (B(actor, 0x390) == 0x13) {
            if (B(actor, 0x38D) == B(scratch, 0x2A8)) {
                B(actor, 0x396) = 0x84;
                B(actor, 0x397) = 0;
            } else {
                func_8005BA30(actor, 0x62, 0x17);
                B(actor, 0x396) = 0x8D;
                B(actor, 0x397) = 1;
            }
        }
    }

after_switch:
    func_800954E8(actor);
    {
        int randOffset = func_8001E6B8(0x24) + 0x20;
        unsigned int deflectAngle;
        int lift;
        int liftParam;

        if (B(actor, 0x3AC) != 0) {
            deflectAngle = B(actor, 0x3AB) + randOffset;
        } else {
            deflectAngle = B(actor, 0x3AB) - randOffset;
        }
        lift = func_8001E6B8(0x10) + 8;
        liftParam = func_80095F40();
        func_8009462C(actor, 0xA, deflectAngle & 0xFF, lift, liftParam);
    }
}
