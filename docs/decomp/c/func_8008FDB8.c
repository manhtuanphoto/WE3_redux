/*
 * func_8008FDB8
 *
 * Purpose:
 *   Handles a goalkeeper diving-parry/save actor state (substate +0x397 = 0 launch,
 *   1 flight/landing across anim frames 2..23, plus post-step parry/deflection
 *   update via func_800954E8 and func_8009462C).
 *
 * Arguments:
 *   actor (u8 *) : pointer to 1000-byte actor struct
 *
 * Return value:
 *   None (void)
 *
 * Actor fields read:
 *   +0x02/+0x04/+0x06 (s16) x/y/z position,
 *   +0x08/+0x0C/+0x10 (s32) x/y/z velocity,
 *   +0x38D (u8) actor id, +0x390 (u8) anim frame,
 *   +0x392 (u8) team slot (0..10), +0x397 (u8) substate,
 *   +0x398 (u8) side (0/1), +0x3AA (u8) body heading,
 *   +0x3AB (u8) target heading, +0x3AC (u8) mirror/foot,
 *   +0x3B9 (u8) stat*10, +0x3C2/+0x3C4/+0x3C6 (s16) target x/y/z
 *
 * Actor fields written:
 *   +0x02/+0x04/+0x06 (s16) x/y/z position,
 *   +0x08/+0x0C/+0x10 (s32) x/y/z velocity,
 *   +0x26 (u16) model rotation, +0x396 (u8) state (0x82, 0x84, or 0x8D),
 *   +0x397 (u8) substate, +0x3A0 (u32) speed (cleared to 0),
 *   +0x3AA (u8) body heading, +0x3D1 (u8) cleared to 0
 *
 * Global / Scratchpad (0x1F800000) fields read:
 *   Scratchpad +0xF0/+0xF2/+0xF4 (s16) initial/scratch xyz,
 *   Scratchpad +0x2A8 (u8) last-touch actor id,
 *   Scratchpad +0x2B0 (u32) ball event code (4 checked),
 *   Scratchpad +0x2F4 (u8 *) pointer to ball actor (reads +0x0C s32 vy),
 *   Scratchpad +0x31C (s16) ball timer (frames since last touch),
 *   D_8010C32F (u8 stat nibble, stride 0x108 per side, 12 per slot)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   Cooldown: 19 - stat_3B9 / 10 frames (at 30 Hz).
 *   Launch horizontal speed: min(dist / 7, 38 + (nibble >> 1)) raw/frame;
 *   vertical velocity: (targetY - scratchY) / 3, gravity/apex adjusted by func_80094474(..., 16, -28 - (nibble >> 1)).
 *   Touch radius at anim frame 7: 0x50 = 80 raw = 0.32 m.
 *   Post-flight damping (frames <2 or >=18): vx, vz scaled by 92/100 per frame.
 *   Anim 0x6E = 110 (dive), anim 0x62 = 98 (recovery at frame 0x17 = 23).
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_8010C32F[];

extern unsigned int func_8001E6B8(int n);
extern int func_8001E694(unsigned int angle, unsigned int scale);
extern int func_8001E5A4(unsigned int angle, unsigned int scale);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, int step);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005BD1C(unsigned char *actor);
extern void func_8005C2AC(unsigned char *actor, int anim, int a2, unsigned char a3, unsigned char a4);
extern void func_8006EC5C(unsigned char *actor);
extern void func_80094474(unsigned char *vyPtr, int frames, int gravity);
extern void func_8009462C(unsigned char *actor, int a1, unsigned int angle, int a3, int a4);
extern int func_80094EEC(unsigned char *actor, int radius);
extern void func_800954E8(unsigned char *actor);
extern void func_80095E8C(unsigned char *actor);
extern unsigned int func_800B1128(int distSq);

void func_8008FDB8(unsigned char *actor)
{
    unsigned int randAng;
    unsigned int parryAngle;
    int arg1;
    int ballVy;
    int vyRandMax;
    int r1;
    int r2;
    int r3;

    switch (B(actor, 0x397)) {
    case 0: {
        int cooldown = 19 - (int)((B(actor, 0x3B9) / 10U) & 0xFF);
        int dx, dz;
        unsigned int dist;
        unsigned int speed;
        unsigned int maxSpeed;
        int tblOff;
        unsigned int ang;
        unsigned int heading;
        int turn;

        if ((int)S(Scratchpad, 0x31C) < cooldown) {
            func_8006EC5C(actor);
            if ((int)S(Scratchpad, 0x31C) == 1) {
                B(actor, 0x396) = 0x82;
                B(actor, 0x397) = 0;
            }
            return;
        }

        func_8005C2AC(actor, 0x6E, 7, B(actor, 0x3AB), B(actor, 0x3AC));

        dx = (int)S(actor, 0x3C2) - (int)S(Scratchpad, 0xF0);
        dz = (int)S(actor, 0x3C6) - (int)S(Scratchpad, 0xF4);
        dist = func_800B1128(dx * dx + dz * dz);
        speed = dist / 7U;

        tblOff = (int)B(actor, 0x392) * 12 + (int)B(actor, 0x398) * 0x108;
        maxSpeed = (unsigned int)(((int)(B(D_8010C32F, tblOff) & 0xF) >> 1) + 0x26);
        if (maxSpeed < speed) {
            speed = maxSpeed;
        }

        ang = func_8005B36C((int)S(Scratchpad, 0xF0), (int)S(Scratchpad, 0xF4), (int)S(actor, 0x3C2), (int)S(actor, 0x3C6)) & 0xFFU;
        SW(actor, 0x08) = func_8001E694(ang, speed);
        SW(actor, 0x10) = func_8001E5A4(ang, speed);
        SW(actor, 0x0C) = ((int)S(actor, 0x3C4) - (int)S(Scratchpad, 0xF2)) / 3;

        tblOff = (int)B(actor, 0x392) * 12 + (int)B(actor, 0x398) * 0x108;
        func_80094474(actor + 0x0C, 0x10, -0x1C - ((int)(B(D_8010C32F, tblOff) & 0xF) >> 1));
        func_8005BA30(actor, 0x6E, 1);

        S(actor, 0x04) = 0;
        B(actor, 0x3D1) = 0;
        W(actor, 0x3A0) = 0;

        heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0x40);
        B(actor, 0x3AA) = (unsigned char)heading;
        turn = 0x1C0 - (int)(heading & 0xFFU);
        H(actor, 0x26) = (unsigned short)((turn % 256) * 16);

        S(actor, 0x02) = (signed short)((int)S(actor, 0x02) + SW(actor, 0x08));
        S(actor, 0x06) = (signed short)((int)S(actor, 0x06) + SW(actor, 0x10));
        B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
        break;
    }

    case 1:
        func_8005BD1C(actor);
        switch (B(actor, 0x390)) {
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
        case 11:
            S(actor, 0x04) = (signed short)((int)S(actor, 0x04) + SW(actor, 0x0C));
            /* fall through */
        case 2:
        case 3:
        case 4: {
            unsigned int heading;
            int turn;
            S(actor, 0x02) = (signed short)((int)S(actor, 0x02) + SW(actor, 0x08));
            S(actor, 0x06) = (signed short)((int)S(actor, 0x06) + SW(actor, 0x10));
            heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
            B(actor, 0x3AA) = (unsigned char)heading;
            turn = 0x1C0 - (int)(heading & 0xFFU);
            H(actor, 0x26) = (unsigned short)((turn % 256) * 16);
            break;
        }

        case 7:
            S(actor, 0x02) = (signed short)((int)S(actor, 0x02) + SW(actor, 0x08));
            S(actor, 0x04) = (signed short)((int)S(actor, 0x04) + SW(actor, 0x0C));
            S(actor, 0x06) = (signed short)((int)S(actor, 0x06) + SW(actor, 0x10));
            if (func_80094EEC(actor, 0x50) != 0) {
                func_80095E8C(actor);
            }
            break;

        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
            S(actor, 0x02) = (signed short)((int)S(actor, 0x02) + SW(actor, 0x08));
            S(actor, 0x04) = (signed short)((int)S(actor, 0x04) - SW(actor, 0x0C));
            S(actor, 0x06) = (signed short)((int)S(actor, 0x06) + SW(actor, 0x10));
            break;

        default:
            S(actor, 0x04) = 0;
            S(actor, 0x02) = (signed short)((int)S(actor, 0x02) + SW(actor, 0x08));
            S(actor, 0x06) = (signed short)((int)S(actor, 0x06) + SW(actor, 0x10));
            SW(actor, 0x08) = (SW(actor, 0x08) * 92) / 100;
            SW(actor, 0x10) = (SW(actor, 0x10) * 92) / 100;
            if (B(actor, 0x390) == 0x17) {
                if (B(actor, 0x38D) == B(Scratchpad, 0x2A8)) {
                    B(actor, 0x396) = 0x84;
                    B(actor, 0x397) = 0;
                } else {
                    func_8005BA30(actor, 0x62, 0x17);
                    B(actor, 0x396) = 0x8D;
                    B(actor, 0x397) = 1;
                }
            }
            break;
        }
        break;

    default:
        break;
    }

    func_800954E8(actor);

    randAng = func_8001E6B8(0x24) + 0x20;
    if (B(actor, 0x3AC) != 0) {
        parryAngle = (unsigned int)B(actor, 0x3AB) + randAng;
    } else {
        parryAngle = (unsigned int)B(actor, 0x3AB) - randAng;
    }

    arg1 = 0xA;
    if (B(actor, 0x390) < 7 && W(Scratchpad, 0x2B0) == 4) {
        arg1 = 0xC;
    }

    r1 = (int)func_8001E6B8(0x10) + 8;
    ballVy = SW((unsigned char *)W(Scratchpad, 0x2F4), 0x0C);
    vyRandMax = ballVy / 736;
    if (vyRandMax < 0) {
        vyRandMax = -vyRandMax;
    }
    r2 = (int)func_8001E6B8(vyRandMax);
    r3 = (int)func_8001E6B8(4);

    func_8009462C(actor, arg1, parryAngle & 0xFFU, r1, -r2 - r3);
}
