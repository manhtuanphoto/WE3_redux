/*
 * func_800391B8
 *
 * Purpose:
 *   AI / player kick decision and execution for passing, crossing, or clearing the ball.
 *   If actor+0x3C2 holds a non-zero target actor id, executes a targeted pass to that
 *   teammate's predicted position. Otherwise chooses between an attacking-third cross/cutback
 *   (signed_x >= 0x23E3), a forward teammate pass (-0x1109 <= signed_x < 0x23E3, not state
 *   0x25/0x28), or a defensive clearance/long ball, applies turn/height/randomness adjustments
 *   to power and lift, kicks the ball via func_8004B0F0, updates ball state/timer/SFX, and
 *   dispatches follow-up pass/camera/stat helpers.
 *
 * Arguments:
 *   actor (unsigned char *) - pointer to 1000-byte player actor struct
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Actor fields:
 *     +0x002 (s16) x (R)
 *     +0x004 (s16) y height (R)
 *     +0x006 (s16) z (R)
 *     +0x38D (u8)  actor id (R on target teammate)
 *     +0x396 (u8)  state (R)
 *     +0x399 (u8)  attack direction flag (R)
 *     +0x39B (u8)  skill/variance flag (R)
 *     +0x3A4 (u8)  move heading (R)
 *     +0x3AB (u8)  target/facing heading (W)
 *     +0x3C2 (s16) target teammate actor id (R/W)
 *   Ball actor (SCRATCH+0x2F4):
 *     +0x002 (s16) x (R)
 *     +0x396 (u8)  state (W: 4)
 *     +0x3B2 (u8)  flag (W: 4)
 *     +0x3C8 (u16) timer/spin (W: 12 - rand%3)
 *     +0x3CA (u16) spin/bounce param (W: rand%2)
 *   Scratchpad (0x1F800000):
 *     +0x036 / +0x066 (s16) predicted/ref x / z (R)
 *     +0x2B0 (u32) ball event code (W: 8 on targeted pass)
 *     +0x2B4 / +0x2B6 (s16) ball target x / z (R)
 *     +0x2F4 (u8*) ptr to ball actor (R)
 *     +0x31C (u16) ball timer (W: attack_dir ^ 1)
 *   Globals:
 *     D_800FF748 (u8[]) actor array (1000-byte stride)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x23E3 = 9187 raw (36.748 m) - attacking third X
 *   0x23E2 = 9186 raw (36.744 m) - attacking third threshold
 *   0x28F0 = 10480 raw (41.92 m) - cross/cutback target X
 *   0x3320 = 13088 raw (52.352 m) - goal line X
 *   -0x1109 = -4361 raw (-17.444 m) - own-half clearance threshold X
 *   -0x22E1 = -8929 raw (-35.716 m) - deep defensive box clearance threshold X
 *   0x300..0x4FF = 768..1279 raw (3.072..5.116 m), 0x400..0x7FF = 1024..2047 raw (4.096..8.188 m)
 *   0x88 = 136 max kick power; 0x508 = kick SFX id
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];

extern unsigned int func_8005B36C(int, int, int, int);
extern int func_8005C7A0(int);
extern unsigned int func_800B1128(unsigned int);
extern int func_8001E6B8(int);
extern void func_800676C4(unsigned int, int, int, int);
extern unsigned int func_80049D94(unsigned char *);
extern unsigned int func_800533F4(unsigned char *, unsigned int, int, int);
extern void func_80056B5C(unsigned char *);
extern unsigned int func_8005B61C(int, int, int, int);
extern void func_80056C60(unsigned char *);
extern void func_8004B0F0(unsigned char *, unsigned int, unsigned int, int);
extern void func_800AB620(int);
extern void func_8004B278(void);
extern void func_80089D00(unsigned char *, unsigned int, int, int);
extern void func_80049A94(unsigned char *, int, int, int, int);

void func_800391B8(unsigned char *actor)
{
    int target_id;
    unsigned char *target_actor;
    int signed_x;
    int target_x;
    int dx;
    unsigned int dist;
    unsigned int kick_ang;
    unsigned int power;
    int lift;
    int variant;
    int lead;
    unsigned int forward_ang;
    unsigned int ang_diff;
    unsigned int tmp_rand;
    unsigned int turn_scale;
    unsigned char *ball;
    int neg_ball_x;
    int pass_x;
    int pass_z;

    target_id = S(actor, 0x3C2);
    if (target_id == 0) {
        signed_x = (B(actor, 0x399) != 0) ? -S(actor, 2) : S(actor, 2);
        if ((short)signed_x >= 0x23E3) {
            target_x = (B(actor, 0x399) != 0) ? -0x28F0 : 0x28F0;
            kick_ang = func_8005B36C(S(actor, 2), S(actor, 6), target_x, 0);
            kick_ang += (unsigned int)func_8005C7A0(0x14);
            dx = (B(actor, 0x399) != 0) ? (S(actor, 2) + 0x28F0) : (S(actor, 2) - 0x28F0);
            dist = func_800B1128((unsigned int)(dx * dx + S(actor, 6) * S(actor, 6)));
            power = (dist / 40U) + (unsigned int)func_8001E6B8(0x10) + 0x20U;
            lift = (int)(dist / 480U) - 10;
            variant = 4;
        } else if ((short)signed_x >= -0x1109 && B(actor, 0x396) != 0x25 && B(actor, 0x396) != 0x28) {
            lead = func_8001E6B8(0x200) + 0x300;
            target_x = (B(actor, 0x399) != 0) ? (S(actor, 2) - lead) : (S(actor, 2) + lead);
            func_800676C4(B(actor, 0x399), (short)target_x, S(actor, 6), 0);

            target_actor = D_800FF748 + ((func_80049D94(actor) & 0xFFU) % 24U) * 1000U;
            dx = S(actor, 2) - S(target_actor, 2);
            {
                int dz = S(actor, 6) - S(target_actor, 6);
                dist = func_800B1128((unsigned int)(dx * dx + dz * dz));
            }
            forward_ang = func_8005B36C(S(actor, 2), S(actor, 6), S(target_actor, 2), S(target_actor, 6));
            if (((forward_ang - (B(actor, 0x399) << 7)) & 0xFFU) < 0x81U) {
                ang_diff = func_8005B36C(S(actor, 2), S(actor, 6), S(target_actor, 2), S(target_actor, 6))
                         - (B(actor, 0x399) << 7);
            } else {
                ang_diff = (B(actor, 0x399) << 7)
                         - func_8005B36C(S(actor, 2), S(actor, 6), S(target_actor, 2), S(target_actor, 6));
            }

            if ((ang_diff & 0xFFU) >= 0x60U) {
                target_x = (B(actor, 0x399) != 0) ? -0x3320 : 0x3320;
                kick_ang = func_8005B36C(S(actor, 2), S(actor, 6), target_x, 0);
                kick_ang += (unsigned int)func_8005C7A0(0x20);
                lift = func_8001E6B8(8) + 4;
                variant = 3;
            } else {
                lead = func_8001E6B8(0x400) + 0x400;
                target_x = (B(actor, 0x399) != 0) ? (S(target_actor, 2) - lead) : (S(target_actor, 2) + lead);
                kick_ang = func_8005B36C(S(actor, 2), S(actor, 6), (short)target_x, S(target_actor, 6));
                variant = (int)(dist >> 8);
                lift = variant - 10;
                power = (dist >> 6) + 0x38U;
                if ((func_800533F4(actor, kick_ang & 0xFFU, 0x10, (int)(dist * dist)) & 0xFFU) != 0) {
                    lift = (variant - 2) + func_8001E6B8(4);
                }
                H(actor, 0x3C2) = B(target_actor, 0x38D);
                variant = 4;
            }
            kick_ang += (unsigned int)func_8005C7A0(8);
        } else {
            signed_x = (B(actor, 0x399) != 0) ? -S(actor, 2) : S(actor, 2);
            if ((short)signed_x >= -0x22E1) {
                target_x = (B(actor, 0x399) != 0) ? -0x3320 : 0x3320;
                kick_ang = func_8005B36C(S(actor, 2), S(actor, 6), target_x, 0);
                kick_ang += (unsigned int)func_8005C7A0(0x10);
            } else {
                if ((B(actor, 0x399) != 0 && S(actor, 6) > 0)
                    || (B(actor, 0x399) == 0 && S(actor, 6) < 0)) {
                    kick_ang = (B(actor, 0x399) << 7) - (unsigned int)(func_8001E6B8(0xC) + 8);
                } else {
                    kick_ang = (B(actor, 0x399) << 7) + (unsigned int)(func_8001E6B8(0xC) + 8);
                }
            }
            lift = func_8001E6B8(5) + 0xE;
            switch (func_8001E6B8(5)) {
            case 3:
                variant = 1;
                break;
            case 4:
                variant = 2;
                break;
            default:
                variant = 0;
                break;
            }
            kick_ang += (unsigned int)func_8005C7A0(8);
        }

        switch (variant) {
        case 0:
            power = (unsigned int)(func_8001E6B8(8) + 0x68);
            lift = (lift - 1) + func_8001E6B8(4);
            break;
        case 1:
            tmp_rand = (unsigned int)func_8001E6B8(0x10);
            power = tmp_rand * 3U + 0x6CU;
            lift = lift - (int)(tmp_rand + (unsigned int)func_8001E6B8(4));
            break;
        case 2:
            tmp_rand = (unsigned int)func_8001E6B8(8);
            power = 0x64U - tmp_rand * 4U;
            lift = (lift - 4) + (int)tmp_rand;
            break;
        case 3:
            power = (unsigned int)(func_8001E6B8(0x20) + 0x40);
            lift = (lift - 2) + func_8001E6B8(6);
            break;
        default:
            break;
        }

        ang_diff = (B(actor, 0x3A4) - kick_ang) & 0xFFU;
        if (ang_diff < 0x81U) {
            turn_scale = 0x100U - (unsigned int)((int)ang_diff >> 2);
        } else {
            turn_scale = 0x100U - (unsigned int)((int)((kick_ang - B(actor, 0x3A4)) & 0xFFU) >> 2);
        }

        if (S(actor, 4) != 0) {
            turn_scale -= 0xCU;
        }

        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        /* UNCLEAR: B(actor, 0x399) is tested, but `negu $v0, $v0` lies in the `beqz` delay slot
         * so both branches compare -S(ball, 2) against -0x23E2. */
        neg_ball_x = -S(ball, 2);
        if (neg_ball_x >= -0x23E2) {
            if (B(actor, 0x39B) != 0) {
                turn_scale -= (unsigned int)func_8001E6B8(0x20);
            }
        }

        power = (unsigned int)((int)power * (int)turn_scale) >> 8;
        if (power >= 0x88U) {
            power = 0x88U;
        }
        func_80056B5C(actor);
    } else {
        target_actor = D_800FF748 + (target_id % 24) * 1000;
        dx = S(actor, 2) - S(target_actor, 2);
        {
            int dz = S(actor, 6) - S(target_actor, 6);
            dist = func_800B1128((unsigned int)(dx * dx + dz * dz));
        }
        kick_ang = func_8005B61C(
            S(SCRATCH, 0x36),
            S(SCRATCH, 0x66),
            (short)(H(target_actor, 2) + (SW(target_actor, 8) << 4)),
            (short)(H(target_actor, 6) + (SW(target_actor, 16) << 4)));
        power = (dist / 52U) + 0x30U;
        variant = (int)(dist / 480U);
        lift = variant - 10;
        if ((func_800533F4(actor, kick_ang & 0xFFU, 0xC, (int)(dist * dist)) & 0xFFU) != 0) {
            lift = (variant - 4) + func_8001E6B8(4);
        }
        func_80056C60(actor);
        if (power >= 0x88U) {
            power = 0x88U;
        }
    }

    func_8004B0F0(actor, kick_ang & 0xFFU, power, lift);

    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    B(ball, 0x3B2) = 4;
    H(ball, 0x3CA) = (unsigned short)func_8001E6B8(2);

    tmp_rand = (unsigned int)func_8001E6B8(3);
    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    H(ball, 0x3C8) = (unsigned short)(12U - tmp_rand);

    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    B(ball, 0x396) = 4;

    H(SCRATCH, 0x31C) = (unsigned short)((B(actor, 0x399) ^ 1U) & 0xFFU);
    func_800AB620(0x508);
    func_8004B278();

    if (S(actor, 0x3C2) != 0) {
        W(SCRATCH, 0x2B0) = 8;
        pass_x = H(target_actor, 2);
        pass_z = H(target_actor, 6);
        func_80089D00(actor, B(target_actor, 0x38D), S(SCRATCH, 0x2B4), S(SCRATCH, 0x2B6));
    } else {
        pass_x = H(SCRATCH, 0x2B4);
        pass_z = H(SCRATCH, 0x2B6);
        func_80089D00(actor, 0, (short)pass_x, (short)pass_z);
    }

    func_80049A94(actor, 1, 1, (short)pass_x, (short)pass_z);
    B(actor, 0x3AB) = (unsigned char)kick_ang;
}
