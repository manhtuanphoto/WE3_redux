/*
 * func_80029F30
 *
 * Purpose:
 *   Executes a player kick/shot/clearance/lob-pass based on actor+0x3C2
 *   (0 = normal pass/shot, 1 = long clearance/pass scaled by goal distance,
 *   2 = high/lobbed clearance scaled by goal distance and ball proximity).
 *   Computes ball speed, loft, and accuracy jitter from player stats,
 *   weak-foot penalty, volley/turning angle, and GK avoidance, calls
 *   func_8004B0F0 to launch the ball, updates ball state/coordinates from
 *   scratch, plays SFX 0x507, and triggers post-kick animation/cooldown.
 *
 * Arguments:
 *   actor (unsigned char *) - pointer to 1000-byte kicker actor struct
 *   power (unsigned int)    - base kick power scale (used as u16 in mode 0)
 *   heading (unsigned int)  - target kick direction byte (0..255)
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Actor fields:
 *     +0x002 (s16) x (R)
 *     +0x006 (s16) z (R)
 *     +0x38D (u8)  actor id (R)
 *     +0x38E (u16) current anim id (R)
 *     +0x392 (u8)  team slot (R)
 *     +0x396 (u8)  state (4 = shooting) (R)
 *     +0x398 (u8)  side 0/1 (R)
 *     +0x399 (u8)  attack direction flag (R)
 *     +0x39A (u8)  charge/power update flag (R)
 *     +0x39B (u8)  balance/pressure penalty flag (R)
 *     +0x3A4 (u8)  move heading (R)
 *     +0x3AC (u8)  kicking foot (R)
 *     +0x3B1 (u8)  spin/curl tag copied to ball+0x3C8 (R)
 *     +0x3B6 (u16) base kick power (R)
 *     +0x3C2 (s16) kick variant (0 = normal, 1 = distance clear, 2 = lob clear) (R)
 *     +0x3C6 (s16) queued charge / loft gauge (R/W)
 *     +0x3D1 (u8)  posture flag set to 3 (W)
 *   Scratchpad (0x1F800000):
 *     +0x036/+0x04E/+0x066 (s16) ball spawn x/y/z copied to ball actor (R)
 *     +0x2F4 (u8*) ptr to ball actor (R); writes ball +0x02/+0x04/+0x06,
 *                  +0x396=4, +0x3B2=2, +0x3BC=1 (if loft>=10), +0x3C8
 *     +0x31C (s16) ball timer, frames since last touch (R/W)
 *     +0x322 (u16[2]) opponent GK actor index table (R)
 *   Globals:
 *     D_800FF748 (u8[])  actor array (1000-byte stride)
 *     D_8010C32C (u32[]) player stat table +2 (side*264 + slot*12; >>4 & 0xF = kick accuracy) (R)
 *     D_8010C330 (u32[]) player stat table +6 (side*264 + slot*12; >>4 & 1 = preferred foot) (R)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x3320 = 13088 raw (52.352 m) - target goal line X
 *   0xFFFFFF = 4096^2 - 1 (~16.384 m), 0x383FFFF = ~7680^2 - 1 (~30.72 m) - post-kick anim distance thresholds
 *   0x600  = 1536 raw (6.144 m) - ball proximity boost in kick variant 2
 *   SFX 0x507 = kick sound
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];
extern unsigned char D_8010C32C[];
extern unsigned char D_8010C330[];

extern int func_8001E6B8(int n);
extern signed short func_8002B1E4(unsigned char *actor);
extern void func_8002B5A4(unsigned char *actor, int anim_mode);
extern void func_8004B0F0(unsigned char *actor, unsigned int heading, unsigned int power, unsigned int lift);
extern void func_80056ED0(unsigned char *actor, unsigned int dist_sq);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8005C7A0(int n);
extern void func_8005CE30(unsigned char actor_id);
extern unsigned int func_80067860(unsigned int side, int x, int z, unsigned int heading, int param4, int param5);
extern void func_800AB620(int sfx_id);
extern unsigned int func_800B1128(int dist_sq);

void func_80029F30(unsigned char *actor, unsigned int power, unsigned int heading)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *ball;
    int charge;
    int gx;
    unsigned int goal_dist;
    int stat_off;
    int kick_acc;
    unsigned int speed;
    int lift;
    unsigned int dist_sq;
    int post_anim;

    if (B(actor, 0x39A) != 0) {
        S(actor, 0x3C6) = func_8002B1E4(actor);
    }

    charge = S(actor, 0x3C6);
    if (charge >= 17) {
        charge = 16;
    }

    gx = (B(actor, 0x399) != 0) ? (S(actor, 2) + 0x3320) : (S(actor, 2) - 0x3320);
    goal_dist = func_800B1128(gx * gx + S(actor, 6) * S(actor, 6));

    stat_off = B(actor, 0x398) * 264 + B(actor, 0x392) * 12;

    switch (S(actor, 0x3C2)) {
    case 1: {
        int rand_scale;
        int rand_loft;
        unsigned int scaled_dist;
        unsigned int out_speed;

        kick_acc = (W(D_8010C32C + stat_off) >> 4) & 0xF;
        rand_scale = func_8001E6B8((9 - kick_acc) * 2) + 6;
        scaled_dist = (unsigned int)(goal_dist * rand_scale) / 10U;

        rand_loft = func_8001E6B8((9 - kick_acc) * 2);
        lift = func_8001E6B8(0x80) + (int)(scaled_dist / (unsigned int)(0x280 - rand_loft)) + 0x16;

        out_speed = ((scaled_dist / (unsigned int)lift) * 7U) / 10U;
        func_8004B0F0(actor, heading & 0xFF, out_speed, (unsigned int)lift);
        break;
    }

    case 2: {
        int jit;
        int rand_loft;
        int rand_spd;
        unsigned int scaled_dist;
        unsigned int ball_dist;
        unsigned int out_speed;

        func_8005CE30(B(actor, 0x38D));
        jit = (signed short)func_8005C7A0(10 - (B(D_8010C32C + stat_off) >> 4));
        scaled_dist = (unsigned int)(goal_dist * (jit + 7)) / 10U;

        kick_acc = (W(D_8010C32C + stat_off) >> 4) & 0xF;
        rand_loft = func_8001E6B8((9 - kick_acc) * 2);
        lift = func_8001E6B8(0x80) + (int)(scaled_dist / (unsigned int)(0x280 - rand_loft)) + 0x10;

        ball = (unsigned char *)W(scratch, 0x2F4);
        {
            int bx = S(actor, 2) - S(ball, 2);
            int bz = S(actor, 6) - S(ball, 6);
            ball_dist = func_800B1128(bx * bx + bz * bz);
        }
        if (ball_dist < 0x601) {
            lift += (int)((0x600U - ball_dist) >> 8);
        }

        rand_spd = func_8001E6B8((9 - kick_acc) * 2);
        out_speed = (((scaled_dist / (unsigned int)lift) * 4U) / 10U) - (unsigned int)(rand_spd - 0x1C);
        func_8004B0F0(actor, heading & 0xFF, out_speed, (unsigned int)lift);
        break;
    }

    default:
        speed = H(actor, 0x3B6) + (unsigned int)charge;
        if (B(actor, 0x396) == 4) {
            int foot_pref;
            unsigned int goal_dir;
            int diff;
            int on_goal;

            speed -= (unsigned int)func_8001E6B8((int)(speed >> 4));
            kick_acc = (W(D_8010C32C + stat_off) >> 4) & 0xF;
            lift = ((charge * 18) / 16) + func_8001E6B8(((charge * 4) / 10) + (9 - kick_acc) * 3);

            foot_pref = (W(D_8010C330 + stat_off) >> 4) & 1;
            if (B(actor, 0x3AC) == foot_pref) {
                if (func_8001E6B8((9 - kick_acc) * 3 + 4) == 0 && (power & 0xFFFF) >= 0x101) {
                    power += 8;
                    lift = func_8001E6B8(8) + 12;
                }
                speed = (unsigned int)((int)speed * (int)(power & 0xFFFF)) >> 8;
            } else {
                unsigned int raw_prod = (unsigned int)((int)speed * (int)(power & 0xFFFF));
                /* 0xF0F0F0F1 mulhi >> 8 corresponds to (raw_prod * 16 / 17) >> 8 */
                speed = (unsigned int)(((unsigned long long)raw_prod * 16ULL) / 17ULL) >> 8;
            }

            if (B(actor, 0x39B) != 0) {
                speed = (speed << 8) / (unsigned int)(func_8001E6B8(12) + 0x110);
            }

            if (H(actor, 0x38E) == 0xF) {
                unsigned int ang_diff = (B(actor, 0x3A4) - heading) & 0xFF;
                int half_ang;
                if (ang_diff < 0x81) {
                    half_ang = (int)ang_diff >> 1;
                } else {
                    half_ang = (int)((heading - B(actor, 0x3A4)) & 0xFF) >> 1;
                }
                speed = (speed * 288U) / (unsigned int)(half_ang + 0x120);
            }

            goal_dir = (unsigned int)(B(actor, 0x399) << 7);
            diff = (int)((goal_dir - heading) & 0xFF);
            if (diff < 0x81) {
                on_goal = (diff < 0x11);
            } else {
                on_goal = ((int)((heading - goal_dir) & 0xFF) < 0x11);
            }
            if (on_goal) {
                if (func_80067860(B(actor, 0x399) ^ 1, S(actor, 2), S(actor, 6), heading & 0xFF, 8, 0x100000) != 0xFF) {
                    unsigned int gk_idx = B(scratch, 0x322 + (B(actor, 0x399) ^ 1) * 2);
                    unsigned char *gk = D_800FF748 + (gk_idx % 24) * 1000;
                    unsigned int gk_ang = func_8005B36C(S(actor, 2), S(actor, 6), S(gk, 2), S(gk, 6));
                    if (((gk_ang - heading) & 0xFF) < 0x81) {
                        heading -= 4;
                    } else {
                        heading += 4;
                    }
                }
            }
        } else {
            speed = (unsigned int)((int)speed * (int)(power & 0xFFFF)) >> 8;
            kick_acc = (W(D_8010C32C + stat_off) >> 4) & 0xF;
            lift = ((charge * 30) / 16) + func_8001E6B8((9 - kick_acc) * 2 + 2);
        }

        func_8004B0F0(actor, heading & 0xFF, speed, (unsigned int)lift);
        ball = (unsigned char *)W(scratch, 0x2F4);
        S(ball, 0x3C8) = B(actor, 0x3B1);
        if (lift >= 10) {
            S(ball, 0x3BC) = 1;
        }
        S(scratch, 0x31C) = (signed short)(B(actor, 0x399) ^ 1);
        break;
    }

    ball = (unsigned char *)W(scratch, 0x2F4);
    S(ball, 2) = S(scratch, 0x36);
    S(ball, 4) = S(scratch, 0x4E);
    S(ball, 6) = S(scratch, 0x66);
    B(ball, 0x396) = 4;

    dist_sq = (unsigned int)((int)goal_dist * (int)goal_dist);
    func_800AB620(0x507);

    ball = (unsigned char *)W(scratch, 0x2F4);
    B(ball, 0x3B2) = 2;
    B(actor, 0x3D1) = 3;

    func_80056ED0(actor, dist_sq);

    if (S(actor, 0x3C2) != 0) {
        post_anim = 0xC;
    } else if (dist_sq <= 0xFFFFFFU) {
        post_anim = 4;
    } else if (dist_sq > 0x383FFFFU) {
        post_anim = 7;
    } else {
        post_anim = 6;
    }
    func_8002B5A4(actor, post_anim);

    if (S(actor, 0x3C2) != 0) {
        S(scratch, 0x31C) += (signed short)(3 + func_8001E6B8(5));
    }
}
