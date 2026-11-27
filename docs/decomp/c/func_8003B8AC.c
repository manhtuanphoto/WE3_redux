/*
 * func_8003B8AC
 *
 * Purpose:
 *   Updates the jumping header / aerial volley state (substate 0 = airborne
 *   rise/fall arc via jtbl_800110BC on anim frame 2..15; substate 1 = landing
 *   slide with 90% velocity decay until anim frame 0x1B -> state 0x11). During
 *   anim frames 2..8, tests ball contact via func_8003D274(actor, 1, 1); on
 *   contact, picks heading via func_8003D99C, computes power (0xA8..0xAB minus
 *   angle/heading penalties) and vertical lift, kicks the ball via func_8004B0F0,
 *   sets ball spin/height flags (+0x3CA, +0x3C8, state=4) and SFX 0x507. Also
 *   applies contact/pushback via func_8009462C when frame >= 2 and not previous
 *   ball owner, and finishes with func_800498AC.
 *
 * Arguments:
 *   actor: (unsigned char*) player actor
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x2A9 (u8)  ball owner actor id (R)
 *     +0x2AA (u8)  previous ball owner actor id (R)
 *     +0x2F4 (u8*) pointer to ball actor (R)
 *   Ball actor (at Scratchpad+0x2F4):
 *     +0x396 (u8)  ball state (W = 4)
 *     +0x3A4 (u8)  ball move heading (R)
 *     +0x3C8 (s16) ball spin/curl parameter (W)
 *     +0x3CA (s16) side-of-body boolean flag (W)
 *   actor:
 *     +0x002 (s16) x position (R/W)
 *     +0x004 (s16) y height (negative = up) (R/W)
 *     +0x006 (s16) z position (R/W)
 *     +0x026 (s16) render rotation ((0x1C0 - bodyHeading) % 256 * 16) (W)
 *     +0x38D (u8)  actor id (R)
 *     +0x390 (u8)  anim frame (R)
 *     +0x396 (u8)  state (W = 0x11 on recovery)
 *     +0x397 (u8)  substate (0=airborne, 1=landing) (R/W)
 *     +0x3AA (u8)  body heading (R/W)
 *     +0x3AB (u8)  target heading (R)
 *     +0x3B2 (u8)  target/aim mode passed to func_8003D99C (R)
 *     +0x3CA (s16) vx step (R/W)
 *     +0x3CC (s16) vy jump velocity (R)
 *     +0x3CE (s16) vz step (R/W)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0xA8  = 168 base header/volley power (<<8 in func_8004B0F0)
 *   0x21  = 33 angle units (~46.4 deg) aligned-ball-heading threshold
 *   0x24  = 36 power reduction when redirecting aligned incoming ball
 *   0x14  = 20 power reduction when actor is not current ball owner
 *   0x507 = header/kick SFX ID
 *   9/10  = 90% per-frame ground slide deceleration in substate 1
 *   0x1B  = frame 27 (0.90 s) -> transition to state 0x11
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern void func_8005BD1C(unsigned char* actor);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, int step);
extern unsigned int func_8003D274(unsigned char* actor, int a1, int a2);
extern unsigned int func_8003D99C(unsigned char* actor, unsigned int a1, int a2);
extern int func_8001E6B8(int n);
extern int func_8002B1E4(unsigned char* actor);
extern void func_8004B0F0(unsigned char* actor, unsigned int heading, int power, int lift);
extern void func_800AB620(int sfxId);
extern void func_8002B5A4(unsigned char* actor, int a1);
extern void func_800571E0(unsigned char* actor);
extern void func_8009462C(unsigned char* actor, int a1, unsigned int heading, int a3, int a4);
extern void func_800498AC(unsigned char* actor);

void func_8003B8AC(unsigned char* actor) {
    unsigned char substate = B(actor, 0x397);

    if (substate == 0) {
        unsigned int heading;
        int turn;
        unsigned char frame;

        func_8005BD1C(actor);
        heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
        B(actor, 0x3AA) = (unsigned char)heading;
        turn = 0x1C0 - (int)(heading & 0xFFU);
        S(actor, 0x26) = (signed short)((turn % 256) * 16);

        S(actor, 0x2) = (signed short)(S(actor, 0x2) + S(actor, 0x3CA));
        S(actor, 0x6) = (signed short)(S(actor, 0x6) + S(actor, 0x3CE));

        frame = B(actor, 0x390);
        switch (frame) {
        case 2:
        case 3:
        case 4:
        case 5:
            S(actor, 0x4) = (signed short)(S(actor, 0x4) + S(actor, 0x3CC));
            break;
        case 6:
            break;
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
            S(actor, 0x4) = (signed short)(S(actor, 0x4) - (S(actor, 0x3CC) >> 1));
            break;
        case 15:
            S(actor, 0x4) = 0;
            B(actor, 0x397)++;
            break;
        }
    } else if (substate == 1) {
        func_8005BD1C(actor);
        S(actor, 0x3CA) = (signed short)((S(actor, 0x3CA) * 9) / 10);
        S(actor, 0x3CE) = (signed short)((S(actor, 0x3CE) * 9) / 10);
        S(actor, 0x6) = (signed short)(S(actor, 0x6) + S(actor, 0x3CE));
        S(actor, 0x2) = (signed short)(S(actor, 0x2) + S(actor, 0x3CA));
        if (B(actor, 0x390) == 0x1B) {
            B(actor, 0x396) = 0x11;
            B(actor, 0x397) = 0;
        }
    }

    if ((unsigned int)(B(actor, 0x390) - 2) < 7U) {
        if ((func_8003D274(actor, 1, 1) & 0xFFU) != 0) {
            unsigned int targetHeading = func_8003D99C(actor, B(actor, 0x3B2), 0);
            int randPow = func_8001E6B8(4);
            unsigned int bodyHeading = B(actor, 0x3AA);
            int power = randPow + 0xA8;
            unsigned int angDiff = (bodyHeading - targetHeading) & 0xFFU;
            unsigned char* ball;
            unsigned int ballDiff;
            int lift;
            int randSpin;
            unsigned int angDiff2;

            if (angDiff < 0x81U) {
                power -= (int)angDiff >> 3;
            } else {
                power -= (int)((targetHeading - bodyHeading) & 0xFFU) >> 3;
            }

            ball = (unsigned char*)W(Scratchpad, 0x2F4);
            ballDiff = (B(ball, 0x3A4) - targetHeading) & 0xFFU;
            if (ballDiff >= 0x81U) {
                ballDiff = (targetHeading - B(ball, 0x3A4)) & 0xFFU;
            }

            if ((int)ballDiff < 0x21) {
                int rLift;
                power -= 0x24;
                rLift = func_8001E6B8(4);
                lift = rLift + func_8002B1E4(actor) + 4;
            } else {
                int rLift;
                if (B(actor, 0x38D) != B(Scratchpad, 0x2A9)) {
                    power -= 0x14;
                }
                rLift = func_8001E6B8(12);
                lift = rLift + func_8002B1E4(actor) - 8;
            }

            func_8004B0F0(actor, targetHeading & 0xFFU, power, lift);

            ball = (unsigned char*)W(Scratchpad, 0x2F4);
            S(ball, 0x3CA) = (signed short)(((targetHeading - B(actor, 0x3AA)) & 0xFFU) < 0x81U);

            ball = (unsigned char*)W(Scratchpad, 0x2F4);
            randSpin = func_8001E6B8(2);
            angDiff2 = (B(actor, 0x3AA) - targetHeading) & 0xFFU;
            if (angDiff2 < 0x81U) {
                S(ball, 0x3C8) = (signed short)((9 - randSpin) - ((int)angDiff2 >> 6));
            } else {
                S(ball, 0x3C8) = (signed short)((9 - randSpin) - ((int)((targetHeading - B(actor, 0x3AA)) & 0xFFU) >> 6));
            }

            B((unsigned char*)W(Scratchpad, 0x2F4), 0x396) = 4;
            func_800AB620(0x507);
            func_8002B5A4(actor, 1);
            func_800571E0(actor);
        }
    }

    if (B(actor, 0x390) >= 2U && B(actor, 0x38D) != B(Scratchpad, 0x2AA)) {
        int jitter = func_8001E6B8(4);
        func_8009462C(actor, 0xA, B(actor, 0x3AA), 0, -4 - jitter);
    }

    func_800498AC(actor);
}
