/*
 * func_8003257C
 *
 * Purpose:
 *   Executes a player's pass or kick/clearance action based on queued pass mode
 *   (actor+0x3C4: 0..6) and charge level (actor+0x3C6). Computes pass target
 *   coordinates, distance, power, lift, and jittered heading, invokes
 *   func_8004B0F0 to launch the ball, updates ball spin/state/timer, and
 *   dispatches receiver selection (mode < 4) or long-ball target projection
 *   (mode >= 4).
 *
 * Arguments:
 *   actor (a0) - pointer to 1000-byte kicking player actor struct
 *
 * Return value:
 *   void
 *
 * Scratchpad (0x1F800000) fields read/written:
 *   0x036, 0x04E, 0x066 (s16, R): ball x, y, z (copied to ball actor after kick)
 *   0x0FE, 0x100 (s16, R/W): computed through-pass target x, z
 *   0x29A (u8, R): match phase (1 = open play)
 *   0x2B0 (u32, W): ball event code (2 = pass, 3 = long ball into box)
 *   0x2B4, 0x2B6 (s16, R): predicted pass landing x, z
 *   0x2F4 (u8 *, R): pointer to ball actor
 *   0x31C (s16, W): ball timer (written = dir ^ 1)
 *   0x31E (u8[], W): nearest/chasing actor ID per attack dir (stride 2)
 *
 * Actor fields read/written:
 *   +0x002, +0x006 (s16, R): actor x, z position (250 raw = 1 m)
 *   +0x38D (u8, R): receiver actor ID
 *   +0x392 (u8, R): team slot (0..10)
 *   +0x396 (u8, W): receiver/ball state (0x1B = 27 on receiver, 4 on ball)
 *   +0x397 (u8, W): receiver substate (cleared to 0)
 *   +0x398 (u8, R): side (0/1)
 *   +0x399 (u8, R): attack direction flag (0 = +X, 1 = -X)
 *   +0x3A4 (u8, R): ball move heading (0..255 = 360 deg)
 *   +0x3AA (u8, R): body heading
 *   +0x3AB (u8, W): receiver facing heading
 *   +0x3B0 (u8, W): ball control timer (set to 8 on ball)
 *   +0x3B1 (u8, W): kicker flag (1) / ball flag
 *   +0x3B2 (u8, W): ball flag (set to 2 on ball)
 *   +0x3BC (s16, W): ball field set to 1 when mode != 6
 *   +0x3C2 (s16, R/W): pass target index/param (0..23 -> target actor; receiver gets 0x10, 0x20, or 0xFF)
 *   +0x3C4 (s16, R): pass mode (0..6; 0/1 = direct pass, 2 = through pass, 3..5 = lofted/cross, 6 = clearance/chip)
 *   +0x3C6 (s16, R/W): charge counter (capped at 16 = 0x10)
 *   +0x3C8 (s16, R/W): target x in mode 0/1; ball spin/timer on ball
 *   +0x3CA (s16, W): ball side/foot flag
 *   +0x3D1 (u8, W): kicker flag (set to 3)
 *
 * Globals read:
 *   D_800C6CAC (u8[]): divisor table indexed by (mode - 4) * 2
 *   D_800C6CAD (u8[]): base power offset table indexed by (mode - 4) * 2
 *   D_800FF748 (u8[]): 1000-byte actor array
 *   D_8010C32E (u16[]): player stat table (stride 12 per slot, 264 per side; & 0xF)
 *
 * Magic numbers & unit conversions (250 raw = 1 m, 30 frames/s):
 *   0x600 (1536 raw = 6.144 m): minimum through-pass lead ahead of target
 *   0x1400 .. 0x1600 (5120 .. 5631 raw = 20.48 .. 22.524 m): random max through-pass lead
 *   0x27E2 (10210 raw = 40.84 m): through-pass X clamp
 *   0x3200 (12800 raw = 51.20 m), 0x1000 (4096 raw = 16.384 m), 0x2100 (8448 raw = 33.792 m): pass distances
 *   0x23E3 (9187 raw = 36.748 m): attacking third X threshold for event code 3
 *   0x12F4 (4852 raw = 19.408 m): penalty box half-width Z threshold for event code 3
 *   0xC0 (192): minimum through-pass power
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800C6CAC[];
extern unsigned char D_800C6CAD[];
extern unsigned char D_800FF748[];
extern unsigned char D_8010C32E[];

extern int func_80054BA4(unsigned char *actor, int param);
extern int func_800B1128(int val);
extern int func_8001E6B8(int n);
extern unsigned int func_8005B61C(int x1, int z1, int x2, int z2);
extern int func_8005C7A0(int n);
extern void func_80056000(unsigned char *actor, unsigned char *target);
extern void func_800676C4(int dir, int x, int z, int param);
extern int func_80049D94(unsigned char *actor);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_8002EB3C(int dist, int param);
extern void func_8004B0F0(unsigned char *actor, unsigned int heading, int power, int lift);
extern void func_8004B278(void);
extern void func_800AB620(int sfx);
extern void func_800512B8(unsigned char *target, int param);
extern void func_80089D00(unsigned char *actor, int target_id, int x, int z);
extern void func_80049A94(unsigned char *actor, int a1, int a2, int x, int z);
extern int func_8001E694(unsigned int angle, int scale);
extern int func_8001E5A4(unsigned int angle, int scale);
extern void func_80035320(unsigned char *actor, int x, int z);

void func_8003257C(unsigned char *actor)
{
    unsigned char *target = D_800FF748;
    unsigned char *ball;
    int dx, dz;
    int dist;
    int pass_dist;
    int max_lead;
    signed short lead_x;
    unsigned int heading;
    int power;
    int lift;
    int stat_lo;
    int rand_val;
    int tmp;
    int idx2;
    unsigned int final_heading;
    unsigned char *new_target;
    int diff;
    int proj_x, proj_z;
    int abs_z;

    if (S(actor, 0x3C6) >= 0x11) {
        S(actor, 0x3C6) = 0x10;
    }

    if (S(actor, 0x3C2) != 0) {
        int res = func_80054BA4(actor, S(actor, 0x3C2) & 0xFF);
        int target_idx;
        S(actor, 0x3C2) = (signed short)res;
        target_idx = (signed short)((signed short)res % 24);
        target = D_800FF748 + target_idx * 1000;
    }

    switch ((unsigned int)S(actor, 0x3C4)) {
    case 0:
        H(actor, 0x3C8) = H(target, 0x2);
        /* fallthrough */
    case 1:
        heading = func_8005B61C(S(Scratchpad, 0x36), S(Scratchpad, 0x66), S(actor, 0x3C8), S(target, 0x6));
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        dx = S(ball, 0x2) - S(actor, 0x3C8);
        dz = S(ball, 0x6) - S(target, 0x6);
        dist = func_800B1128(dx * dx + dz * dz);
        pass_dist = (int)(((unsigned int)dist * 12U) / 10U);
        if (pass_dist >= 0x3201) {
            pass_dist = 0x3200;
        }

        /* UNCLEAR: dead quotient computed in original binary (side-effect of func_8001E6B8 + div) */
        (void)(pass_dist / (func_8001E6B8(8) + 0x4C));

        stat_lo = H(D_8010C32E, B(actor, 0x392) * 12 + B(actor, 0x398) * 264) & 0xF;
        rand_val = func_8001E6B8(9 - stat_lo);
        power = rand_val + 0x20;
        tmp = pass_dist / power;
        /* UNCLEAR: delay slot of jal func_8005C7A0 computes lift using rand_val before func_8005C7A0 returns */
        lift = (tmp / 2) + rand_val;
        power += (signed short)func_8005C7A0((signed short)((9 - stat_lo) * 2 + 2));

        func_800676C4(B(actor, 0x399), S(actor, 0x3C8), S(target, 0x6), 0);
        target = D_800FF748 + ((func_80049D94(actor) & 0xFF) % 24U) * 1000;
        B(target, 0x396) = 0x1B;
        B(target, 0x397) = 0;
        S(target, 0x3C2) = 0xFF;
        func_80056000(actor, target);
        break;

    case 2:
        dx = S(actor, 0x2) - S(target, 0x2);
        dz = S(actor, 0x6) - S(target, 0x6);
        dist = func_800B1128(dx * dx + dz * dz);
        pass_dist = (int)(((unsigned int)dist << 3) / 20U) + 0x600;
        max_lead = func_8001E6B8(0x200) + 0x1400;
        if (max_lead < pass_dist) {
            pass_dist = max_lead;
        }

        if (B(actor, 0x399) != 0) {
            lead_x = (signed short)(S(target, 0x2) - pass_dist);
        } else {
            lead_x = (signed short)(S(target, 0x2) + pass_dist);
        }
        S(Scratchpad, 0xFE) = lead_x;

        if (B(actor, 0x399) != 0) {
            if (-lead_x >= 0x27E3) {
                S(Scratchpad, 0xFE) = -0x27E2;
            }
        } else {
            if (lead_x >= 0x27E3) {
                S(Scratchpad, 0xFE) = 0x27E2;
            }
        }

        lead_x = S(Scratchpad, 0xFE);
        if (B(actor, 0x399) != 0) {
            if (-lead_x < -(S(target, 0x2) + 0x600)) {
                S(Scratchpad, 0xFE) = (signed short)(S(target, 0x2) - 0x600);
            }
        } else {
            if (lead_x < S(target, 0x2) + 0x600) {
                S(Scratchpad, 0xFE) = (signed short)(S(target, 0x2) + 0x600);
            }
        }

        S(Scratchpad, 0x100) = S(target, 0x6);
        heading = func_8005B61C(S(Scratchpad, 0x36), S(Scratchpad, 0x66), S(Scratchpad, 0xFE), S(Scratchpad, 0x100));

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        dx = S(ball, 0x2) - S(Scratchpad, 0xFE);
        dz = S(ball, 0x6) - S(Scratchpad, 0x100);
        dist = func_800B1128(dx * dx + dz * dz);

        stat_lo = H(D_8010C32E, B(actor, 0x392) * 12 + B(actor, 0x398) * 264) & 0xF;
        rand_val = func_8001E6B8((9 - stat_lo) >> 1);
        pass_dist = (int)(((unsigned int)(dist * (rand_val + 11))) / 10U);

        /* UNCLEAR: dead quotient computed in original binary */
        (void)(pass_dist / (func_8001E6B8(8) + 0x4D));

        stat_lo = H(D_8010C32E, B(actor, 0x392) * 12 + B(actor, 0x398) * 264) & 0xF;
        rand_val = func_8001E6B8(9 - stat_lo);
        power = rand_val + 0x22;
        tmp = pass_dist / power;
        /* UNCLEAR: delay slot of jal func_8005C7A0 computes lift using rand_val before func_8005C7A0 returns */
        lift = (tmp / 2) + rand_val;
        stat_lo = H(D_8010C32E, B(actor, 0x392) * 12 + B(actor, 0x398) * 264) & 0xF;
        power += (signed short)func_8005C7A0((signed short)((9 - stat_lo) * 2 + 8));
        if (power < 0xC0) {
            power = 0xC0;
        }

        func_80056000(actor, target);
        func_800676C4(B(actor, 0x399), S(Scratchpad, 0xFE), S(Scratchpad, 0x100), 0);
        new_target = D_800FF748 + ((func_80049D94(actor) & 0xFF) % 24U) * 1000;
        if (target != new_target) {
            target = new_target;
            B(target, 0x396) = 0x1B;
            S(target, 0x3C2) = 0x10;
        } else {
            unsigned int to_tgt = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(Scratchpad, 0xFE), S(Scratchpad, 0x100));
            unsigned int dir_ang = B(actor, 0x399) << 7;
            if (((to_tgt - dir_ang) & 0xFF) < 0x81U) {
                B(target, 0x3AB) = (unsigned char)(dir_ang + 0x70);
            } else {
                B(target, 0x3AB) = (unsigned char)(dir_ang - 0x70);
            }
            B(target, 0x396) = 0x1B;
            S(target, 0x3C2) = 0x20;
        }
        B(target, 0x397) = 0;
        break;

    case 3:
        if ((B(Scratchpad, 0x29A) & 0xFF) != 1U) {
            heading = B(actor, 0x3AA);
        } else {
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            heading = B(ball, 0x3A4);
        }
        pass_dist = S(actor, 0x3C6) * 640 + 0x800;
        if (pass_dist >= 0x3201) {
            pass_dist = 0x3200;
        }
        power = (pass_dist / 88) + 0x20;
        stat_lo = H(D_8010C32E, B(actor, 0x392) * 12 + B(actor, 0x398) * 264) & 0xF;
        tmp = pass_dist / power;
        rand_val = func_8001E6B8(9 - stat_lo);
        lift = (tmp / 2) + rand_val;
        power += (signed short)func_8005C7A0((signed short)((9 - stat_lo) * 2 + 2));
        break;

    case 6:
        pass_dist = S(actor, 0x3C6) * 640 + 0x800;
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        heading = B(ball, 0x3A4);
        if (pass_dist < 0x1000) {
            pass_dist = 0x1000;
        }
        if (pass_dist >= 0x3201) {
            pass_dist = 0x3200;
        }
        power = func_8002EB3C(pass_dist, func_8001E6B8(0x800) + 0x2800) & 0xFF;
        lift = pass_dist / 512;
        /* Delay slot sets pass_dist = 0x2100 unconditionally before branch to common tail */
        pass_dist = 0x2100;
        if (lift >= 0xB) {
            lift = 0xA;
        }
        break;

    case 4:
    case 5:
    default:
        pass_dist = S(actor, 0x3C6) * 640 + 0x800;
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        heading = B(ball, 0x3A4);
        if (pass_dist >= 0x3201) {
            pass_dist = 0x3200;
        }
        idx2 = (S(actor, 0x3C4) - 4) * 2;
        power = (pass_dist / B(D_800C6CAC, idx2)) + B(D_800C6CAD, idx2);
        stat_lo = H(D_8010C32E, B(actor, 0x392) * 12 + B(actor, 0x398) * 264) & 0xF;
        tmp = pass_dist / power;
        rand_val = func_8001E6B8(9 - stat_lo);
        lift = (tmp / 2) + rand_val;
        power += (signed short)func_8005C7A0((signed short)((9 - stat_lo) * 2 + 2));
        break;
    }

    stat_lo = H(D_8010C32E, B(actor, 0x392) * 12 + B(actor, 0x398) * 264) & 0xF;
    final_heading = heading + func_8005C7A0((signed short)(9 - stat_lo));
    heading = final_heading;

    func_8004B0F0(actor, heading & 0xFF, power, lift);

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    H(ball, 0x2) = H(Scratchpad, 0x36);
    H(ball, 0x4) = H(Scratchpad, 0x4E);
    H(ball, 0x6) = H(Scratchpad, 0x66);
    H(ball, 0x3CA) = (((final_heading - B(actor, 0x3AA)) & 0xFF) < 0x81U) ? 1 : 0;

    rand_val = func_8001E6B8(2);
    diff = B(actor, 0x3AA) - final_heading;
    if ((unsigned int)(diff & 0xFF) < 0x81U) {
        tmp = (diff & 0xFF) >> 6;
    } else {
        tmp = ((final_heading - B(actor, 0x3AA)) & 0xFF) >> 6;
    }
    H(ball, 0x3C8) = (unsigned short)((12 - rand_val) - tmp);
    B(ball, 0x396) = 4;

    B(actor, 0x3B1) = 1;
    if (S(actor, 0x3C4) != 6) {
        H(ball, 0x3BC) = 1;
        func_8004B278();
    }

    func_800AB620(0x507);
    S(Scratchpad, 0x31C) = (signed short)(B(actor, 0x399) ^ 1);

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    B(ball, 0x3B0) = 8;
    B(ball, 0x3B2) = 2;
    B(actor, 0x3D1) = 3;
    W(Scratchpad, 0x2B0) = 2;

    if (S(actor, 0x3C4) < 4) {
        func_800676C4(B(actor, 0x399), S(Scratchpad, 0x2B4), S(Scratchpad, 0x2B6), 0);
        if (S(actor, 0x3C2) == 0) {
            target = D_800FF748 + ((func_80049D94(actor) & 0xFF) % 24U) * 1000;
            B(target, 0x396) = 0x1B;
            B(target, 0x397) = 0;
            H(target, 0x3C2) = 0xFF;
        }
        func_800512B8(target, 1);
        B(Scratchpad, 0x31E + (B(actor, 0x399) << 1)) = B(target, 0x38D);
        func_80089D00(actor, B(target, 0x38D), S(Scratchpad, 0x2B4), S(Scratchpad, 0x2B6));
        func_80049A94(actor, 2, 1, S(Scratchpad, 0x2B4), S(Scratchpad, 0x2B6));
    } else {
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        proj_x = (signed short)(H(ball, 0x2) + func_8001E694(heading & 0xFF, pass_dist));
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        proj_z = (signed short)(H(ball, 0x6) + func_8001E5A4(heading & 0xFF, pass_dist));

        if ((B(actor, 0x399) != 0 && -proj_x >= 0x23E3) ||
            (B(actor, 0x399) == 0 && proj_x >= 0x23E3)) {
            abs_z = (proj_z < 0) ? -proj_z : proj_z;
            if (abs_z < 0x12F4) {
                W(Scratchpad, 0x2B0) = 3;
            }
        }
        func_80035320(actor, proj_x, proj_z);
    }
}
