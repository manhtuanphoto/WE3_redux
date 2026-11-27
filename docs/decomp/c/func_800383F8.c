/*
 * func_800383F8
 * Purpose: Updates a player actor's aerial / header / volley / jump-clear state machine
 *          across substates 0..3 (+0x397), including jump timing against predicted ball height,
 *          in-air movement, ball strike on anim frame 4, and landing recovery.
 * Arguments:
 *   - actor (unsigned char *): pointer to 1000-byte player actor struct
 * Return value: void
 * Actor fields read/written:
 *   - +0x02 (s16): X position (250 raw = 1 m)
 *   - +0x06 (s16): Z position (250 raw = 1 m)
 *   - +0x26 (s16/u16): model rotation ((0x1C0 - heading) % 256) * 16
 *   - +0x38D (u8): actor ID
 *   - +0x390 (u8): current animation frame
 *   - +0x392 (u8): team slot (0..10)
 *   - +0x394 (u32): packed state/substate word (upper 16 bits == 0x270000 forces substate = 2)
 *   - +0x396 (u8): actor state (transitions to 0, 1, 0xE, 0x1B, 0x27, 0x2A)
 *   - +0x397 (u8): substate (0..3, 0x10, 0x18, 0x50)
 *   - +0x398 (u8): team side (0/1)
 *   - +0x399 (u8): attack direction flag (0/1)
 *   - +0x39A (u8): AI/special action flag (transitions to state 0xE / substate 0x50)
 *   - +0x3A7 (u8): set to 1
 *   - +0x3AA (u8): body heading
 *   - +0x3AB (u8): target heading
 *   - +0x3B2 (u8): pass/shot target selector
 *   - +0x3C2 (s16): target actor index (cleared if state == 0x28, else from func_800381F4)
 *   - +0x3C4 (s16): jump/reach threshold = ((stat + 100) * 320) / 180
 *   - +0x3CA (s16): in-air X step per frame
 *   - +0x3CE (s16): in-air Z step per frame
 *   - +0x3D1 (u8): set to 1 in substate 0
 * Scratchpad reads/writes:
 *   - 0x1F80003E (s16): predicted ball X
 *   - 0x1F800056 (s16): predicted ball height at earlier tick
 *   - 0x1F800058 (s16): predicted ball height at later tick
 *   - 0x1F80006E (s16): predicted ball Z
 *   - 0x1F8000F2 (s16): written with -(jumpHeight) - 0x40
 *   - 0x1F8002F4 (ptr): pointer to ball actor (+0x0C s32 Y velocity, +0x396 set to 4 on strike)
 *   - 0x1F80031C (s16): ball timer (frames since last touch)
 *   - 0x1F80031E[side] (u8): primary/nearest teammate actor ID for attack side
 * Globals read:
 *   - D_800FF748 (actor[24]): 1000-byte actor array
 *   - D_8010C32C / D_8010C330: player stat words indexed by side*264 + slot*12
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - -0x40 (-64 raw = -0.256 m, negative Y is up), +0x60 (+96 raw = 0.384 m): height window offsets
 *   - 0xFFFF (256^2 = 1.024^2 m^2), 0x3FFFF (512^2 = 2.048^2 m^2): teammate proximity thresholds
 *   - -0x21E2 (-8674 raw = -34.70 m), 0x12F4 (4852 raw = 19.41 m): penalty-box X/Z bounds
 *   - 0x507: ball strike sound effect ID
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCHPAD ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010C32C[];
extern unsigned char D_8010C330[];

extern void func_8005BD1C(unsigned char *);
extern void func_8005B908(unsigned char *);
extern void func_80038F18(unsigned char *, int, int, int);
extern unsigned int func_800381F4(unsigned char *);
extern unsigned int func_8005B36C(int, int, int, int);
extern int func_80038C4C(unsigned char *, unsigned int);
extern unsigned int func_8005B7CC(unsigned int, unsigned int, unsigned int);
extern int func_8003D274(unsigned char *, int, int);
extern unsigned int func_8003D99C(unsigned char *, unsigned int, int);
extern int func_8005C7A0(int);
extern int func_8001E6B8(int);
extern int func_8002B1E4(unsigned char *);
extern void func_8004B0F0(unsigned char *, unsigned int, int, int);
extern void func_8005715C(unsigned char *);
extern void func_8002B5A4(unsigned char *, int);
extern void func_800AB620(int);
extern void func_800391B8(unsigned char *);
extern void func_8005BA30(unsigned char *, int, int);
extern void func_800498AC(unsigned char *);

void func_800383F8(unsigned char *actor)
{
    unsigned char *scratch = SCRATCHPAD;
    unsigned char substate;
    int fallIntoSub1 = 0;

    B(actor, 0x3A7) = 1;
    if ((W(actor, 0x394) & 0xFFFF0000U) == 0x270000U) {
        B(actor, 0x397) = 2;
    }

    substate = B(actor, 0x397);
    switch (substate) {
    case 0: {
        unsigned int keyId;
        unsigned int row;
        int stat;
        int jumpY;
        int diffY;

        B(actor, 0x3D1) = 1;
        func_8005BD1C(actor);

        if (S(scratch, 0x31C) == 1) {
            B(actor, 0x396) = 1;
            B(actor, 0x397) = 0;
            func_8005B908(actor);
            break;
        }

        keyId = B(scratch, 0x31E + B(actor, 0x399) * 2);
        if (B(actor, 0x38D) != keyId) {
            unsigned char *keyActor = D_800FF748 + (keyId % 24) * 1000;
            int dx = S(actor, 0x02) - S(keyActor, 0x02);
            int dz = S(actor, 0x06) - S(keyActor, 0x06);
            if (dx * dx + dz * dz > 0xFFFF) {
                goto check_jump;
            }
            B(actor, 0x396) = 0x1B;
            B(actor, 0x397) = 0x18;
            func_8005B908(actor);
            break;
        }

    check_jump:
        row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        stat = (int)(((W(D_8010C330, row) >> 11) & 0x7F) + 100);
        jumpY = (-(stat * 424)) / 180 - 0x40;
        S(scratch, 0xF2) = (signed short)jumpY;

        if (S(scratch, 0x56) < (signed short)jumpY) {
            row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
            stat = (int)(((W(D_8010C330, row) >> 11) & 0x7F) + 100);
            diffY = S(scratch, 0x56) - ((-(stat * 424)) / 180);
            if (diffY < 0) {
                diffY = -diffY;
            }
            if (diffY >= 0x40) {
                func_80038F18(actor, 0, 0, 0);
                func_8005B908(actor);
                break;
            }
        }
        fallIntoSub1 = 1;
    }
    /* fall through into case 1 when fallIntoSub1 */
    case 1: {
        signed short predX;
        signed short predZ;
        signed short targetIdx;
        unsigned int dir;
        unsigned char *ball;
        unsigned int row;
        int stat;
        int xSigned;
        int absZ;

        (void)fallIntoSub1;
        predX = S(scratch, 0x3E);
        predZ = S(scratch, 0x6E);

        if (B(actor, 0x396) == 0x28) {
            H(actor, 0x3C2) = 0;
        } else {
            H(actor, 0x3C2) = (unsigned short)(func_800381F4(actor) & 0xFF);
        }

        targetIdx = S(actor, 0x3C2);
        if (targetIdx == 0) {
            dir = (unsigned int)B(actor, 0x399) << 7;
        } else {
            unsigned char *targetActor = D_800FF748 + (targetIdx % 24) * 1000;
            dir = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(targetActor, 0x02), S(targetActor, 0x06));
        }

        if (func_80038C4C(actor, dir & 0xFF) != 0) {
            B(actor, 0x397) = 2;
            break;
        }

        if (S(scratch, 0x58) < S(scratch, 0xF2) + 0x60) {
            func_80038F18(actor, 0, 0, 0);
            func_8005B908(actor);
            break;
        }

        ball = (unsigned char *)W(scratch, 0x2F4);
        if (SW(ball, 0x0C) > 0) {
            unsigned int keyId = B(scratch, 0x31E + B(actor, 0x399) * 2);
            if (B(actor, 0x38D) != keyId) {
                unsigned char *keyActor = D_800FF748 + (keyId % 24) * 1000;
                int kdx = S(keyActor, 0x02) - predX;
                int kdz = S(keyActor, 0x06) - predZ;
                int adx = S(actor, 0x02) - predX;
                int adz = S(actor, 0x06) - predZ;
                int akdx = S(actor, 0x02) - S(keyActor, 0x02);
                int akdz = S(actor, 0x06) - S(keyActor, 0x06);

                if (kdx * kdx + kdz * kdz < adx * adx + adz * adz || akdx * akdx + akdz * akdz <= 0x3FFFF) {
                    B(actor, 0x396) = 0x1B;
                    B(actor, 0x397) = 0x18;
                    func_8005B908(actor);
                    break;
                }
            }
        }

        if (B(actor, 0x396) == 0x28) {
            xSigned = S(actor, 0x02);
            if (B(actor, 0x399) != 0) {
                xSigned = -xSigned;
            }
            if (xSigned < -0x21E2) {
                absZ = S(actor, 0x06);
                if (absZ < 0) {
                    absZ = -absZ;
                }
                if (absZ < 0x12F4) {
                    B(actor, 0x396) = 0x2A;
                    B(actor, 0x397) = 0;
                    func_8005B908(actor);
                    break;
                }
            }
        }

        if (B(actor, 0x39A) != 0) {
            B(actor, 0x396) = 0x0E;
            B(actor, 0x397) = 0x50;
            func_8005B908(actor);
            break;
        }

        row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        stat = (int)(((W(D_8010C330, row) >> 11) & 0x7F) + 100);
        B(actor, 0x396) = 0x1B;
        B(actor, 0x397) = 0x10;
        H(actor, 0x3C4) = (unsigned short)((stat * 320) / 180);
        func_8005B908(actor);
        break;
    }

    case 2: {
        unsigned int heading;
        int rot;

        func_8005BD1C(actor);
        H(actor, 0x02) = H(actor, 0x02) + H(actor, 0x3CA);
        H(actor, 0x06) = H(actor, 0x06) + H(actor, 0x3CE);
        heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0x10);
        B(actor, 0x3AA) = (unsigned char)heading;
        rot = 0x1C0 - (int)(heading & 0xFF);
        H(actor, 0x26) = (unsigned short)((rot % 256) * 16);

        if (B(actor, 0x390) == 4) {
            B(actor, 0x397) = B(actor, 0x397) + 1;
            if (func_8003D274(actor, 1, 1) != 0) {
                if (B(actor, 0x396) == 0x27) {
                    unsigned int kickDir;
                    int jitter;
                    unsigned int row;
                    unsigned int stat;
                    int randPart;
                    int power;
                    unsigned int bodyAng;
                    unsigned int diff;
                    int angPenalty;
                    int liftRand;
                    int liftBase;
                    unsigned char *ball;

                    kickDir = func_8003D99C(actor, B(actor, 0x3B2), 0);
                    jitter = (signed short)func_8005C7A0(8);
                    row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
                    stat = (W(D_8010C32C, row) >> 20) & 0xF;
                    randPart = func_8001E6B8(9 - (int)stat) - 0xAC;
                    power = jitter - randPart;

                    bodyAng = B(actor, 0x3AA);
                    diff = (bodyAng - kickDir) & 0xFF;
                    if (diff < 0x81U) {
                        angPenalty = (signed int)diff >> 2;
                    } else {
                        angPenalty = (signed int)((kickDir - bodyAng) & 0xFF) >> 2;
                    }
                    power -= angPenalty;

                    liftRand = func_8005C7A0(2);
                    liftBase = (signed short)func_8002B1E4(actor);
                    func_8004B0F0(actor, kickDir & 0xFF, power, liftBase + liftRand - 4);

                    ball = (unsigned char *)W(scratch, 0x2F4);
                    B(ball, 0x396) = 4;
                    func_8005715C(actor);
                    func_8002B5A4(actor, 0);
                    func_800AB620(0x507);
                } else {
                    func_800391B8(actor);
                }
            }
        }
        break;
    }

    case 3: {
        unsigned int heading;
        int rot;

        func_8005BD1C(actor);
        heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 8);
        B(actor, 0x3AA) = (unsigned char)heading;
        rot = 0x1C0 - (int)(heading & 0xFF);
        H(actor, 0x26) = (unsigned short)((rot % 256) * 16);

        if (B(actor, 0x390) == 0) {
            func_8005BA30(actor, 1, 0);
            B(actor, 0x396) = 0;
            B(actor, 0x397) = 0;
        }
        break;
    }
    }

    func_800498AC(actor);
}
