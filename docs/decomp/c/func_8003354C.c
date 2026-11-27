/*
 * func_8003354C
 *
 * Purpose:
 *   Evaluates an incoming ball / target player interaction for an actor: if the
 *   actor is flagged (+0x39A != 0) and faces a target actor in state 0x82 within
 *   +-56 angle units (~79 deg) and within sqrt(0x1440000) = 4608 raw units
 *   (18.432 m) with clear path check func_800533F4 == 0, transitions the actor
 *   into state 5 (substate 1) targeting that actor. Otherwise computes the best
 *   intercept/action angle via func_80033B88 and either prepares a first-time
 *   kick/trap animation (anim 0x0F, state +0x397 = 2) or executes a touch/trap
 *   (func_8002CE60, state +0x397 = 3) depending on relative ball angle sector.
 *
 * Arguments:
 *   actor (u8 *) - pointer to 1000-byte actor struct
 *
 * Return value:
 *   void
 *
 * Actor fields read:
 *   +0x02 (s16) x, +0x06 (s16) z, +0x38D (u8) actor id,
 *   +0x392 (u8) team slot, +0x396 (u8) state (on target), +0x398 (u8) side,
 *   +0x39A (u8) active/target flag, +0x3AA (u8) body heading,
 *   +0x3C4 (u16) intercept/action angle
 *
 * Actor fields written:
 *   +0x396 (u8) state (5 if engaging target), +0x397 (u8) substate (1, 2, or 3),
 *   +0x3AB (u8) target heading, +0x3AC (u8) mirror/foot,
 *   +0x3C2 (u16) target actor id or previous body heading,
 *   +0x3C4 (u16) cleared to 0 or set to func_80033B88 result,
 *   +0x3D1 (u8) set to 2 when func_8005385C fails
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x40 (s16) ball/target x, +0x70 (s16) ball/target z,
 *   +0x2F4 (u32 -> u8 *) pointer to ball actor
 *
 * Scratchpad fields written:
 *   +0x2A8 (u8) last-touch actor id,
 *   ball+0x396 (u8) set to 3 on touch/trap
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x39 (57 angle units ~80.2 deg) - target engagement cone half-angle (< 57)
 *   0x40 (64 angle units = 90 deg) - ball angle cone half-angle (< 64)
 *   0x143FFFF (21233663; dist^2 < 0x1440000, dist < 4608 raw = 18.432 m)
 *   0x10 (16), 0x0C (12) - turn / check step parameters
 *   0x1A (26) - parameter to func_80049DF0
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned int D_8010C330[];

extern unsigned int func_8005CE14(unsigned char actor_id);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_800533F4(unsigned char *actor, unsigned int angle, int param);
extern unsigned int func_80033B88(unsigned char *actor);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, int step);
extern void func_80049DF0(unsigned char *actor, unsigned int angle, int param2, int param3);
extern int func_8005385C(unsigned char *actor, int param);
extern void func_8002CE60(unsigned char *actor, unsigned int angle1, unsigned int angle2);

void func_8003354C(unsigned char *actor)
{
    unsigned int angle;
    unsigned int diff;
    int in_cone;
    unsigned int ball_angle;
    unsigned int sector;
    unsigned int turned;

    if (B(actor, 0x39A) != 0) {
        unsigned int idx = func_8005CE14(B(actor, 0x38D)) & 0xFF;
        unsigned char *target = D_800FF748 + (idx % 24) * 1000;

        angle = func_8005B36C(S(actor, 2), S(actor, 6), S(target, 2), S(target, 6));
        if (B(target, 0x396) == 0x82) {
            unsigned char body_hdg = B(actor, 0x3AA);

            diff = (angle - body_hdg) & 0xFF;
            if (diff < 0x81U) {
                in_cone = ((int)diff < 0x39);
            } else {
                in_cone = ((int)((body_hdg - angle) & 0xFF) < 0x39);
            }

            if (in_cone) {
                int dx = S(actor, 2) - S(target, 2);
                int dz = S(actor, 6) - S(target, 6);
                unsigned int dist_sq = (unsigned int)(dx * dx + dz * dz);

                if (dist_sq <= 0x143FFFFU) {
                    if ((func_800533F4(actor, angle & 0xFF, 0x10) & 0xFF) == 0) {
                        H(actor, 0x3C4) = 0;
                        B(actor, 0x396) = 5;
                        B(actor, 0x397) = 1;
                        H(actor, 0x3C2) = B(target, 0x38D);
                        return;
                    }
                }
            }
        }
    }

    H(actor, 0x3C4) = (unsigned short)(func_80033B88(actor) & 0xFF);
    H(actor, 0x3C2) = B(actor, 0x3AA);

    ball_angle = func_8005B36C(S(actor, 2), S(actor, 6), S(Scratchpad, 0x40), S(Scratchpad, 0x70));
    {
        unsigned char body_hdg = B(actor, 0x3AA);
        diff = (ball_angle - body_hdg) & 0xFF;
        if (diff < 0x81U) {
            in_cone = ((int)diff < 0x40);
        } else {
            in_cone = ((int)((body_hdg - ball_angle) & 0xFF) < 0x40);
        }
        if (!in_cone) {
            ball_angle = B(actor, 0x3AA);
        }
    }

    sector = (((B(actor, 0x3C4) - ball_angle) + 8) & 0xFF) >> 4;
    switch (sector & 0xFF) {
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        if (func_8005385C(actor, 1) == 0) {
            B(actor, 0x3D1) = 2;
        } else {
            unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
            B(Scratchpad, 0x2A8) = B(actor, 0x38D);
            B(ball, 0x396) = 3;
            func_8002CE60(actor, B(actor, 0x3C4), ball_angle & 0xFF);
            B(actor, 0x397) = 3;
        }
        break;

    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    default:
        if ((sector & 0xFF) < 2U || (sector & 0xFF) == 15U) {
            unsigned int off = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
            B(actor, 0x3AC) = (unsigned char)((W((unsigned char *)D_8010C330, off) >> 4) & 1);
        } else {
            B(actor, 0x3AC) = (unsigned char)(((sector & 0xFF) >> 3) & 1);
        }
        func_8005BA30(actor, 0xF, 0x16);
        turned = func_8005B7CC(B(actor, 0x3C4), ball_angle & 0xFF, 0xC);
        func_80049DF0(actor, turned & 0xFF, 0x1A, 4);
        B(actor, 0x3AB) = (unsigned char)turned;
        B(actor, 0x397) = 2;
        break;
    }
}
