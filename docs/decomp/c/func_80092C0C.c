/*
 * func_80092C0C - Goalkeeper diving punch / clear action (animation 0x6A)
 *
 * Purpose:
 *   Executes the goalkeeper's diving punch/clear sequence. In substate 0, sets
 *   facing away from the ball's heading, computes velocity toward predicted ball
 *   intercept (+0x3C2/3C4/3C6) clamped by GK stat, initiates vertical arc via
 *   func_80094474, and starts anim 0x6A. In substate 1, advances animation frames
 *   1..23: turns toward target heading, updates position/height, punches the ball
 *   at frame 9 if within 1.024 m (0x100), resets height at frame 17, transitions to
 *   state 0x82 at frame 23, and performs late collision/hand check.
 *
 * Arguments:
 *   actor - pointer to 1000-byte goalkeeper actor struct
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Actor fields read & written:
 *   Scratch 0xF0/F2/F4 (s16) - limb world xyz from func_8005C2AC
 *   Scratch 0x2B4/2B6  (s16) - ball landing/target coordinates after kick
 *   Scratch 0x2F4      (ptr) - ball actor pointer
 *   Actor +0x02/04/06  (s16) - x, y (height), z position
 *   Actor +0x08/0C/10  (s32) - x, y, z velocity
 *   Actor +0x26        (s16) - model rotation
 *   Actor +0x390       (u8)  - anim frame (1..23)
 *   Actor +0x392       (u8)  - team slot (0..10)
 *   Actor +0x396       (u8)  - state (0x82 at frame 23)
 *   Actor +0x397       (u8)  - substate (0 -> 1, 0 at frame 23)
 *   Actor +0x398       (u8)  - side (0/1)
 *   Actor +0x399       (u8)  - attack direction flag (0/1)
 *   Actor +0x3AA/3AB   (u8)  - body heading / target heading
 *   Actor +0x3AC       (u8)  - mirror/foot flag (0)
 *   Actor +0x3C2/3C4/3C6 (s16) - intercept target x, y, z
 *   D_8010C32F         (u8[]) - player stat nibble table (stride 12 per slot, 264 per side)
 *
 * Magic numbers:
 *   0x6A   (106)   = GK diving punch animation id
 *   0x38   (56)    = base horizontal dive speed cap (0.224 m/frame)
 *   0x100  (256)   = 1.024 m touch radius for punch at frame 9
 *   0x80..0x8F     = kick power (128 + rand%16)
 *   0x10..0x13     = kick lift (16 + rand%4)
 *   736            = ball y-velocity fixed-point scale factor
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char D_8010C32F[];

extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);
extern void func_8005C2AC(unsigned char *actor, unsigned int anim, unsigned int frame, unsigned int heading, unsigned int mirror);
extern unsigned int func_800B1128(unsigned int val);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8001E694(unsigned int angle, int scale);
extern int func_8001E5A4(unsigned int angle, int scale);
extern void func_80094474(unsigned char *vy_ptr, int arg1, int gravity);
extern void func_8005BA30(unsigned char *actor, unsigned int anim, unsigned int frame);
extern void func_8005BD1C(unsigned char *actor);
extern int func_80094EEC(unsigned char *actor, unsigned int radius);
extern int func_8005C7A0(int n);
extern int func_8001E6B8(int n);
extern void func_8004B0F0(unsigned char *actor, unsigned int heading, unsigned int power, unsigned int lift);
extern void func_80095090(unsigned char *actor);
extern void func_80057A94(unsigned char *actor);
extern void func_8004B278(unsigned char *actor);
extern void func_80049A94(unsigned char *actor, int arg1, int arg2, int x, int z);
extern void func_8009462C(unsigned char *actor, int arg1, unsigned int angle, int arg3, int arg4);
extern void func_800954E8(unsigned char *actor);

void func_80092C0C(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *ball;
    int abs_vy;
    int jitter;

    if (B(actor, 0x397) == 0) {
        unsigned char *ball0 = (unsigned char *)W(scratch, 0x2F4);
        unsigned int cur_ang = (B(ball0, 0x3A4) + 0x80) & 0xFF;
        unsigned int target_ang = (B(actor, 0x399) << 7) & 0x80;
        unsigned int dist, speed, max_speed, stat, move_ang;
        int dx, dz;

        B(actor, 0x3AB) = (unsigned char)func_8005B7CC(target_ang, cur_ang, 0x18);
        B(actor, 0x3AC) = 0;
        func_8005C2AC(actor, 0x6A, 9, B(actor, 0x3AB), 0);

        dx = S(actor, 0x3C2) - S(scratch, 0xF0);
        dz = S(actor, 0x3C6) - S(scratch, 0xF4);
        dist = func_800B1128((unsigned int)(dx * dx + dz * dz));
        speed = dist / 10;

        stat = B(D_8010C32F, B(actor, 0x392) * 12 + B(actor, 0x398) * 264) & 0xF;
        max_speed = (stat >> 1) + 0x38;
        if (speed > max_speed) {
            speed = max_speed;
        }

        move_ang = func_8005B36C(S(scratch, 0xF0), S(scratch, 0xF4),
                                 S(actor, 0x3C2), S(actor, 0x3C6)) & 0xFF;
        SW(actor, 8) = func_8001E694(move_ang, (int)speed);

        SW(actor, 0xC) = (S(actor, 0x3C4) - S(scratch, 0xF2)) / 5;
        stat = B(D_8010C32F, B(actor, 0x392) * 12 + B(actor, 0x398) * 264) & 0xF;
        func_80094474(actor + 0xC, 0xFF, (signed short)(-10 - (int)stat));

        SW(actor, 0x10) = func_8001E5A4(move_ang, (int)speed);
        func_8005BA30(actor, 0x6A, 0);

        H(actor, 2) = (unsigned short)(H(actor, 2) + SW(actor, 8));
        H(actor, 6) = (unsigned short)(H(actor, 6) + SW(actor, 0x10));
        B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
    } else if (B(actor, 0x397) == 1) {
        unsigned int frame_idx;

        func_8005BD1C(actor);
        frame_idx = (unsigned int)(B(actor, 0x390) - 1);

        switch (frame_idx) {
        case 4:
        case 5:
        case 6:
        case 7:
        case 9:
            H(actor, 4) = (unsigned short)(H(actor, 4) + SW(actor, 0xC));
            /* fall through */
        case 0:
        case 1:
        case 2:
        case 3: {
            unsigned int ang = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
            int turn = 0x1C0 - (int)(ang & 0xFF);
            B(actor, 0x3AA) = (unsigned char)ang;
            H(actor, 0x26) = (unsigned short)((turn % 256) << 4);
            H(actor, 2) = (unsigned short)(H(actor, 2) + SW(actor, 8));
            H(actor, 6) = (unsigned short)(H(actor, 6) + SW(actor, 0x10));
            break;
        }

        case 8: {
            unsigned int ang = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
            int turn = 0x1C0 - (int)(ang & 0xFF);

            B(actor, 0x3AA) = (unsigned char)ang;
            H(actor, 0x26) = (unsigned short)((turn % 256) << 4);
            H(actor, 2) = (unsigned short)(H(actor, 2) + SW(actor, 8));
            H(actor, 4) = (unsigned short)(H(actor, 4) + SW(actor, 0xC));
            H(actor, 6) = (unsigned short)(H(actor, 6) + SW(actor, 0x10));

            if (func_80094EEC(actor, 0x100) != 0) {
                unsigned int kick_dir = func_8005B7CC((B(actor, 0x399) << 7) & 0x80, B(actor, 0x3AA), 0x20);
                int pwr, lift;

                kick_dir = (unsigned int)((int)kick_dir + func_8005C7A0(0x10));
                pwr = func_8001E6B8(0x10) + 0x80;
                lift = func_8001E6B8(4) + 0x10;
                func_8004B0F0(actor, kick_dir & 0xFF, (unsigned int)pwr, (unsigned int)lift);
                func_80095090(actor);
                func_80057A94(actor);
                func_8004B278(actor);
                func_80049A94(actor, 1, 1, S(scratch, 0x2B4), S(scratch, 0x2B6));
            }
            break;
        }

        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            H(actor, 2) = (unsigned short)(H(actor, 2) + SW(actor, 8));
            H(actor, 4) = (unsigned short)(H(actor, 4) - SW(actor, 0xC));
            H(actor, 6) = (unsigned short)(H(actor, 6) + SW(actor, 0x10));
            break;

        case 16:
            H(actor, 4) = 0;
            break;

        case 22:
            B(actor, 0x396) = 0x82;
            B(actor, 0x397) = 0;
            break;
        }
    }

    jitter = (signed short)func_8005C7A0(4);
    ball = (unsigned char *)W(scratch, 0x2F4);
    abs_vy = SW(ball, 0xC) / 736;
    if (abs_vy < 0) {
        abs_vy = -abs_vy;
    }
    func_8009462C(actor, 0xC, (B(ball, 0x3A4) + 0x80) & 0xFF, 0, jitter - abs_vy);
    func_800954E8(actor);
}
