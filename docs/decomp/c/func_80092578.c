/*
 * func_80092578
 *
 * Purpose:
 *   Goalkeeper dive/leap state handler (anim 0x63). In substate 0, optionally
 *   aborts into state 0x96 if the shot/ball moves away before launch, otherwise
 *   computes dive velocity toward predicted intercept (+0x3C2/+0x3C4/+0x3C6
 *   from scratch 0xF0/0xF2/0xF4), starts anim 0x63, and enters substate 1.
 *   In substate 1, steps animation frames 1..23: rotates body, integrates xyz,
 *   tests ball catch/touch (func_80094EEC radius 0x50 = 0.32 m) on frames 9..13,
 *   applies air/ground drag (94/100 and 8/10), and at frame 23 transitions to
 *   state 0x84 (if owner/last-touch) or 0x8D (recovery anim 0x62). Also updates
 *   GK reach/hands via func_800954E8 and func_8009462C.
 *
 * Arguments:
 *   actor (unsigned char *): goalkeeper's 1000-byte actor struct
 *
 * Return value:
 *   void
 *
 * Scratchpad / Actor fields read or written:
 *   Scratch reads:  0xF0/0xF2/0xF4 (s16 xyz), 0x2A8 (last-touch actor id),
 *                   0x2F4 (ptr to ball actor), 0x31C (s16 ball timer)
 *   Actor reads:    +0x02/+0x04/+0x06 (xyz), +0x08/+0x0C/+0x10 (vel xyz),
 *                   +0x38D (id), +0x390 (anim frame), +0x392 (team slot),
 *                   +0x397 (substate), +0x398 (side), +0x3A0 (speed),
 *                   +0x3AB (target heading), +0x3AC (mirror/side), +0x3B9 (stat),
 *                   +0x3C2/+0x3C4/+0x3C6 (dive target xyz), +0x3D0
 *   Actor writes:   +0x02/+0x04/+0x06 (xyz), +0x08/+0x0C/+0x10 (vel xyz),
 *                   +0x26 (rotation), +0x396 (state), +0x397 (substate),
 *                   +0x3A4 (move heading), +0x3AA (body heading)
 *
 * Magic numbers / units (250 raw = 1 m, 30 frames/s):
 *   0x50   = 80 raw (0.32 m) GK catch radius in func_80094EEC
 *   0x24/0x2A = 36/42 base max dive speed (raw/frame) + (stat & 0xF)/2
 *   -0x1A  = -26 base vertical peak/clamp offset
 *   0x63   = 99 (GK dive anim ID); 0x62 = 98 (GK recovery anim ID)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned char D_8010C32F[];

void func_8006EC5C(unsigned char *actor);
unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
void func_8005C2AC(unsigned char *actor, int anim, int a2, int heading, int mirror);
unsigned int func_800B1128(unsigned int val);
int func_8001E694(unsigned int angle, unsigned int scale);
int func_8001E5A4(unsigned int angle, unsigned int scale);
void func_80094474(int *vy_ptr, int a1, int min_vy);
void func_8005BA30(unsigned char *actor, int anim, int frame);
unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);
void func_8005BD1C(unsigned char *actor);
int func_80094EEC(unsigned char *actor, int radius);
void func_80095E8C(unsigned char *actor);
void func_800954E8(unsigned char *actor);
unsigned int func_8001E6B8(unsigned int n);
int func_80095F40(void);
void func_8009462C(unsigned char *actor, int a1, int a2, int a3, int a4);

void func_80092578(unsigned char *actor)
{
    unsigned char substate = B(actor, 0x397);
    unsigned int rand_ang;
    int target_ang;
    int a1_param;
    int a3_param;
    int v_ret;

    if (substate == 0) {
        int threshold = 22 - (int)((B(actor, 0x3B9) / 10) & 0xFF);
        if (S(SCRATCH, 0x31C) < threshold && B(actor, 0x3D0) != 0) {
            func_8006EC5C(actor);
            if (S(SCRATCH, 0x31C) != 1) {
                return;
            }
            {
                unsigned int ang = func_8005B36C(S(actor, 2), S(actor, 6), S(actor, 0x3C2), S(actor, 0x3C6));
                unsigned char *ball = (unsigned char *)W(SCRATCH, 0x2F4);
                unsigned char ball_dir = B(ball, 0x3A4);
                unsigned int diff = (ang - ball_dir) & 0xFF;
                int aligned;
                if (diff < 0x81) {
                    aligned = (int)diff < 0x40;
                } else {
                    aligned = (int)((ball_dir - ang) & 0xFF) < 0x40;
                }
                if (!aligned) {
                    B(actor, 0x396) = 0x96;
                    B(actor, 0x397) = 0;
                    return;
                }
            }
        }

        func_8005C2AC(actor, 0x63, 9, B(actor, 0x3AB), B(actor, 0x3AC));

        {
            int dx = S(actor, 0x3C2) - S(SCRATCH, 0xF0);
            int dz = S(actor, 0x3C6) - S(SCRATCH, 0xF4);
            unsigned int dist = func_800B1128((unsigned int)(dx * dx + dz * dz));
            unsigned int spd = dist / 10;
            unsigned char slot = B(actor, 0x392);
            unsigned char side = B(actor, 0x398);
            int stat_half = (int)(B(D_8010C32F, slot * 12 + side * 264) & 0xF) >> 1;
            unsigned int max_spd = (unsigned int)(stat_half + ((W(actor, 0x3A0) < 0x30) ? 0x24 : 0x2A));
            unsigned int ang;
            unsigned int body_h;
            int rot_raw;

            if (max_spd < spd) {
                spd = max_spd;
            }

            ang = func_8005B36C(S(SCRATCH, 0xF0), S(SCRATCH, 0xF4), S(actor, 0x3C2), S(actor, 0x3C6));
            B(actor, 0x3A4) = (unsigned char)ang;
            SW(actor, 8) = func_8001E694(ang & 0xFF, spd);
            SW(actor, 0xC) = (S(actor, 0x3C4) - S(SCRATCH, 0xF2)) / 5;

            stat_half = (int)(B(D_8010C32F, B(actor, 0x392) * 12 + B(actor, 0x398) * 264) & 0xF) >> 1;
            func_80094474(&SW(actor, 0xC), 0xFF, -0x1A - stat_half);

            SW(actor, 0x10) = func_8001E5A4(ang & 0xFF, spd);
            func_8005BA30(actor, 0x63, 0);

            body_h = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0x40);
            B(actor, 0x3AA) = (unsigned char)body_h;
            rot_raw = 0x1C0 - (int)(body_h & 0xFF);
            S(actor, 0x26) = (signed short)((rot_raw % 256) << 4);
            S(actor, 4) = 0;
            S(actor, 2) = (signed short)(H(actor, 2) + SW(actor, 8));
            S(actor, 6) = (signed short)(H(actor, 6) + SW(actor, 0x10));
            B(actor, 0x397)++;
        }
    } else if (substate == 1) {
        unsigned char frame;
        func_8005BD1C(actor);
        frame = B(actor, 0x390);
        switch (frame) {
        case 5:
        case 6:
        case 7:
        case 8:
            S(actor, 4) = (signed short)(H(actor, 4) + SW(actor, 0xC));
            /* fallthrough */
        case 1:
        case 2:
        case 3:
        case 4: {
            unsigned int body_h = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
            int rot_raw = 0x1C0 - (int)(body_h & 0xFF);
            B(actor, 0x3AA) = (unsigned char)body_h;
            S(actor, 0x26) = (signed short)((rot_raw % 256) << 4);
            S(actor, 2) = (signed short)(H(actor, 2) + SW(actor, 8));
            S(actor, 6) = (signed short)(H(actor, 6) + SW(actor, 0x10));
            break;
        }

        case 9:
            S(actor, 2) = (signed short)(H(actor, 2) + SW(actor, 8));
            S(actor, 4) = (signed short)(H(actor, 4) + SW(actor, 0xC));
            S(actor, 6) = (signed short)(H(actor, 6) + SW(actor, 0x10));
            if (func_80094EEC(actor, 0x50) != 0) {
                func_80095E8C(actor);
            }
            SW(actor, 0xC) >>= 1;
            break;

        case 10:
        case 11:
        case 12:
        case 13:
            if (func_80094EEC(actor, 0x50) != 0) {
                func_80095E8C(actor);
            }
            /* fallthrough */
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
            S(actor, 2) = (signed short)(H(actor, 2) + SW(actor, 8));
            S(actor, 6) = (signed short)(H(actor, 6) + SW(actor, 0x10));
            S(actor, 4) = (signed short)(H(actor, 4) - SW(actor, 0xC));
            SW(actor, 8) = (SW(actor, 8) * 94) / 100;
            SW(actor, 0x10) = (SW(actor, 0x10) * 94) / 100;
            break;

        default:
            S(actor, 4) = 0;
            S(actor, 2) = (signed short)(H(actor, 2) + SW(actor, 8));
            S(actor, 6) = (signed short)(H(actor, 6) + SW(actor, 0x10));
            SW(actor, 8) = (SW(actor, 8) * 8) / 10;
            SW(actor, 0x10) = (SW(actor, 0x10) * 8) / 10;
            if (B(actor, 0x390) == 0x17) {
                if (B(actor, 0x38D) == B(SCRATCH, 0x2A8)) {
                    B(actor, 0x396) = 0x84;
                    B(actor, 0x397) = 0;
                    return;
                }
                func_8005BA30(actor, 0x62, 0x17);
                B(actor, 0x396) = 0x8D;
                B(actor, 0x397) = 1;
            }
            break;
        }
    }

    func_800954E8(actor);
    rand_ang = func_8001E6B8(0x24) + 0x20;
    if (B(actor, 0x3AC) != 0) {
        target_ang = (int)B(actor, 0x3AB) + (int)rand_ang;
    } else {
        target_ang = (int)B(actor, 0x3AB) - (int)rand_ang;
    }
    a1_param = (B(actor, 0x390) == 9) ? 0xA : 0xC;
    a3_param = (int)func_8001E6B8(0x10) + 8;
    v_ret = func_80095F40();
    func_8009462C(actor, a1_param, target_ang & 0xFF, a3_param, v_ret);
}
