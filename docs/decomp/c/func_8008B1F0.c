/*
 * func_8008B1F0
 *
 * Purpose:
 *   Per-frame goalkeeper diving / sliding save animation handler. Advances the
 *   animation (func_8005BD1C), updates position/velocity/rotation by animation
 *   frame (1..7: turn toward target heading and move at full velocity; 8..15:
 *   decelerate by 92/100 and check ball catch/deflect within radius 0xC0;
 *   16..34 and default: decelerate by 85/100; 35: transition to anim 0x5D,
 *   state 0x85, substate 0), then checks secondary body-contact ball collision
 *   via func_8009462C.
 *
 * Arguments:
 *   actor (u8 *) - pointer to 1000-byte goalkeeper actor struct
 *
 * Return value:
 *   void
 *
 * Actor fields read:
 *   +0x02 (s16) x, +0x06 (s16) z, +0x08 (s32) vx, +0x10 (s32) vz,
 *   +0x38D (u8) actor id, +0x390 (u8) anim frame, +0x3A0 (u32) speed,
 *   +0x3AA (u8) body heading, +0x3AB (u8) target heading,
 *   +0x3AC (u8) mirror/foot, +0x3CE (s16) contact-handled flag
 *
 * Actor fields written:
 *   +0x02 (s16) x, +0x06 (s16) z, +0x08 (s32) vx, +0x10 (s32) vz,
 *   +0x26 (s16) rotation, +0x396 (u8) state (0x85 at frame 35),
 *   +0x397 (u8) substate (0 at frame 35), +0x3AA (u8) body heading,
 *   +0x3CE (s16) set to 1 on ball contact
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x36 (s16) ball x, +0x66 (s16) ball z, +0x2AA (u8) previous ball owner
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x08 (8 angle units = 11.25 deg/frame) - body turn step in frames 1..7
 *   92 / 100 - horizontal velocity damping factor in frames 8..15
 *   85 / 100 - horizontal velocity damping factor in frames 16..34 & default
 *   0xC0 (192 raw = 0.768 m) - ball touch test radius in frames 8..15
 *   0x10 (16 angle units = 22.5 deg) - deflection angle offset from +0x3AB
 *   0x0C (12) - contact radius/parameter to func_8009462C
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern void func_8005BD1C(unsigned char *actor);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, int step);
extern int func_80094EEC(unsigned char *actor, int radius);
extern int func_80094DC4(unsigned char *actor);
extern int func_80095F40(unsigned char *actor);
extern void func_800951C4(unsigned char *actor, unsigned int angle, int power, unsigned int speed);
extern void func_80095090(unsigned char *actor);
extern void func_80057968(unsigned char *actor);
extern void func_80094FF4(unsigned char *actor);
extern void func_800578E0(unsigned char *actor);
extern void func_8008B518(unsigned char *actor);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_800954E8(unsigned char *actor);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8009462C(unsigned char *actor, int param1, unsigned int dir, unsigned int speed, int power);

void func_8008B1F0(unsigned char *actor)
{
    unsigned int angle;
    unsigned char target_hdg;
    int dir;
    int power;

    func_8005BD1C(actor);

    switch (B(actor, 0x390)) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    {
        unsigned int heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 8);
        int turn = 0x1C0 - (int)(heading & 0xFF);
        B(actor, 0x3AA) = (unsigned char)heading;
        H(actor, 0x26) = (unsigned short)((turn % 256) * 16);
        H(actor, 2) = (unsigned short)(H(actor, 2) + SW(actor, 8));
        H(actor, 6) = (unsigned short)(H(actor, 6) + SW(actor, 0x10));
        break;
    }

    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    {
        int vx = SW(actor, 8);
        int vz = SW(actor, 0x10);
        H(actor, 2) = (unsigned short)(H(actor, 2) + vx);
        SW(actor, 8) = (vx * 92) / 100;
        H(actor, 6) = (unsigned short)(H(actor, 6) + vz);
        SW(actor, 0x10) = (vz * 92) / 100;

        if (S(actor, 0x3CE) == 0 && func_80094EEC(actor, 0xC0) != 0) {
            if (func_80094DC4(actor) != 0 && B(actor, 0x38D) != B(Scratchpad, 0x2AA)) {
                int deflect = B(actor, 0x3AC) ? (B(actor, 0x3AB) + 0x10) : (B(actor, 0x3AB) - 0x10);
                int pwr = func_80095F40(actor);
                func_800951C4(actor, deflect & 0xFF, pwr, W(actor, 0x3A0));
                func_80095090(actor);
                func_80057968(actor);
                S(actor, 0x3CE) = 1;
            } else {
                func_80094FF4(actor);
                func_800578E0(actor);
            }
            func_8008B518(actor);
        }
        break;
    }

    case 35:
        func_8005BA30(actor, 0x5D, 4);
        B(actor, 0x396) = 0x85;
        B(actor, 0x397) = 0;
        break;

    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    default:
    {
        int vx = SW(actor, 8);
        int vz = SW(actor, 0x10);
        H(actor, 2) = (unsigned short)(H(actor, 2) + vx);
        SW(actor, 8) = (vx * 85) / 100;
        H(actor, 6) = (unsigned short)(H(actor, 6) + vz);
        SW(actor, 0x10) = (vz * 85) / 100;
        break;
    }
    }

    func_800954E8(actor);
    angle = func_8005B36C(S(actor, 2), S(actor, 6), S(Scratchpad, 0x36), S(Scratchpad, 0x66));
    target_hdg = B(actor, 0x3AB);
    if (((angle - target_hdg) & 0xFF) < 0x81U) {
        dir = target_hdg - 0x10;
    } else {
        dir = target_hdg + 0x10;
    }
    power = func_80095F40(actor);
    if ((func_8009462C(actor, 0xC, dir & 0xFF, W(actor, 0x3A0), power) & 0xFF) != 0) {
        S(actor, 0x3CE) = 1;
        func_8008B518(actor);
    }
}
