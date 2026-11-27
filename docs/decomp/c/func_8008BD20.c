/*
 * func_8008BD20
 *
 * Purpose:
 *   Searches the 10 outfield teammates following `actor` (+0x3E8 stride) for
 *   the best pass target. Filters by availability, X-position progression,
 *   distance-squared window [0x190000 .. 0x2400000], openness score
 *   (func_800495B8 > 0x8FFFFF), and clear passing lane (func_800533F4),
 *   computing pass lead/power in actor+0x3C6.
 *
 * Arguments:
 *   actor (u8*): passing player actor pointer (10 teammates follow at +0x3E8)
 *
 * Return value:
 *   Actor ID (B(best_teammate, 0x38D)) of the chosen teammate, or 0 if none found.
 *
 * Actor fields read:
 *   +0x002 (s16) X position
 *   +0x006 (s16) Z position
 *   +0x38D (u8)  actor ID
 *   +0x399 (u8)  team side (0 or 1; determines +/-0x400 lead X and target facing 0/128)
 *
 * Actor fields written:
 *   +0x3C6 (s16) computed pass power/lead offset (capped at 0xC00)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned int func_80052C6C(unsigned char *teammate);
extern unsigned int func_800495B8(unsigned char *teammate);
extern unsigned int func_8005B36C(signed short x1, signed short z1, signed short x2, signed short z2);
extern unsigned int func_800533F4(unsigned char *actor, unsigned int ang, int param2, unsigned int dist2);
extern unsigned int func_800B1128(unsigned int val);

unsigned char func_8008BD20(unsigned char *actor)
{
    unsigned char *teammate = actor + 0x3E8;
    unsigned char *best_teammate = 0;
    unsigned int best_score = 0;
    int i;

    for (i = 0; i < 10; i++, teammate += 0x3E8) {
        signed short ax, tx, abs_ax, abs_tx;
        signed int dx, dz;
        unsigned int dist2;
        unsigned int openness;
        signed short lead_x;
        unsigned int pass_ang;
        signed short tz, abs_tz;
        unsigned int score;
        unsigned int ref_ang;
        unsigned int ang_diff;
        unsigned int ok_straight;

        if ((func_80052C6C(teammate) & 0xFFU) != 0) {
            continue;
        }

        tx = S(teammate, 2);
        ax = S(actor, 2);
        abs_tx = (tx >= 0) ? tx : (signed short)-tx;
        abs_ax = (ax >= 0) ? ax : (signed short)-ax;
        if (abs_ax < abs_tx) {
            continue;
        }

        dx = ax - tx;
        dz = S(actor, 6) - S(teammate, 6);
        dist2 = (unsigned int)(dx * dx + dz * dz);
        if ((dist2 - 0x190000U) > 0x2270000U) {
            continue;
        }

        openness = func_800495B8(teammate);
        if (openness <= 0x8FFFFFU) {
            continue;
        }

        lead_x = (B(actor, 0x399) != 0)
            ? (signed short)(S(teammate, 2) - 0x400)
            : (signed short)(S(teammate, 2) + 0x400);
        pass_ang = func_8005B36C(S(actor, 2), S(actor, 6), lead_x, S(teammate, 6));

        if ((func_800533F4(actor, pass_ang & 0xFFU, 0x10, dist2) & 0xFFU) != 0) {
            continue;
        }

        tz = S(teammate, 6);
        abs_tz = (tz >= 0) ? tz : (signed short)-tz;
        score = (unsigned int)(abs_tz + 0x3320 - abs_tx);
        if (score < best_score) {
            continue;
        }

        ref_ang = (unsigned int)B(actor, 0x399) << 7;
        ang_diff = (pass_ang - ref_ang) & 0xFFU;
        if (ang_diff < 0x81U) {
            ok_straight = (ang_diff < 8U);
        } else {
            ok_straight = (((ref_ang - pass_ang) & 0xFFU) < 8U);
        }

        if (ok_straight) {
            S(actor, 0x3C6) = 0;
        } else {
            signed short p = (signed short)((func_800B1128(openness) - 0x200U) >> 1);
            unsigned int d_sqrt;
            if (p >= 0xC01) {
                p = 0xC00;
            }
            S(actor, 0x3C6) = p;
            d_sqrt = func_800B1128(dist2) >> 1;
            if (d_sqrt < (unsigned int)(signed int)S(actor, 0x3C6)) {
                S(actor, 0x3C6) = (signed short)d_sqrt;
            }
        }

        best_teammate = teammate;
        best_score = score;
    }

    if (best_score == 0) {
        return 0;
    }
    return B(best_teammate, 0x38D);
}
