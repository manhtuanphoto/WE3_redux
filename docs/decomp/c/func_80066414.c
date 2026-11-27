/*
 * func_80066414
 *
 * Purpose:
 *   Evaluates whether teammate/target actor a1 is an open forward pass option for
 *   actor a0. Checks facing-angle cone (+-0x28), Z separation (< 0x69A), pitch X
 *   bounds (|x| < 0x27E3), minimum squared distance (>= 0x1000), and verifies that
 *   none of the 10 opponent outfielders (D_800FF748[opp*1000 + 0x3E8..]) is ahead
 *   of a1 and within +-0x1F of a1's attack direction.
 *
 * Arguments:
 *   passer (unsigned char *): pointer to passer actor struct (0x3E8 bytes)
 *   target (unsigned char *): pointer to candidate receiver actor struct (0x3E8 bytes)
 *
 * Return value:
 *   1 if target passes all openness/geometry checks; 0 otherwise.
 *
 * Actor fields read:
 *   +0x00 (u8) active flag (on opponent outfielders),
 *   +0x02 (s16) X position, +0x06 (s16) Z position,
 *   +0x38D (u8) team/slot id passed to func_8005CE30,
 *   +0x399 (u8) team side (0 -> attack +X, nonzero -> attack -X),
 *   +0x3AA (u8) facing angle
 *
 * Global fields read:
 *   D_800FF748 (u8[]): actor array (1000-byte stride; +0x3E8 skips GK slot 0)
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define S(p, o) (*(signed short *)((p) + (o)))

extern unsigned char D_800FF748[];

extern unsigned int func_8005B36C(signed short x0, signed short z0, signed short x1, signed short z1);
extern unsigned int func_8005CE30(unsigned char teamSlot);

int func_80066414(unsigned char *passer, unsigned char *target)
{
    unsigned int ang;
    unsigned char facing;
    int dz;
    int abs_dz;
    signed short tx;
    int abs_tx;
    int dx;
    int sideSign;
    unsigned int oppIdx;
    unsigned char *opp;
    int i;

    if (passer == target) {
        return 0;
    }

    ang = func_8005B36C(S(passer, 2), S(passer, 6), S(target, 2), S(target, 6));
    facing = B(passer, 0x3AA);
    if (((ang - facing) & 0xFF) < 0x81U) {
        ang = func_8005B36C(S(passer, 2), S(passer, 6), S(target, 2), S(target, 6));
        if (((ang - B(passer, 0x3AA)) & 0xFF) >= 0x29) {
            return 0;
        }
    } else {
        ang = func_8005B36C(S(passer, 2), S(passer, 6), S(target, 2), S(target, 6));
        if (((B(passer, 0x3AA) - ang) & 0xFF) >= 0x29) {
            return 0;
        }
    }

    dz = (int)S(passer, 6) - (int)S(target, 6);
    abs_dz = (dz >= 0) ? dz : -dz;
    if (abs_dz >= 0x69A) {
        return 0;
    }

    tx = S(target, 2);
    abs_tx = (tx >= 0) ? (int)tx : -(int)tx;
    if (abs_tx >= 0x27E3) {
        return 0;
    }

    dx = (int)S(passer, 2) - (int)tx;
    if ((unsigned int)(dx * dx + dz * dz) < 0x1000U) {
        return 0;
    }

    sideSign = (B(passer, 0x399) != 0) ? -1 : 1;
    oppIdx = func_8005CE30(B(passer, 0x38D)) & 0xFF;
    opp = D_800FF748 + oppIdx * 1000 + 0x3E8;

    for (i = 0; (i & 0xFF) < 0xA; i++, opp += 0x3E8) {
        signed short ox;
        unsigned int defAng;
        unsigned int oppDir;

        if (B(opp, 0) == 0) {
            continue;
        }

        ox = S(opp, 2);
        if (sideSign * (int)ox < sideSign * (int)S(target, 2)) {
            continue;
        }

        defAng = func_8005B36C(S(target, 2), S(target, 6), ox, S(opp, 6));
        oppDir = (unsigned int)B(target, 0x399) << 7;
        if (((oppDir - defAng) & 0xFF) < 0x81U) {
            defAng = func_8005B36C(S(target, 2), S(target, 6), ox, S(opp, 6));
            oppDir = (unsigned int)B(target, 0x399) << 7;
            if ((int)((oppDir - defAng) & 0xFF) < 0x20) {
                return 0;
            }
        } else {
            defAng = func_8005B36C(S(target, 2), S(target, 6), ox, S(opp, 6));
            oppDir = (unsigned int)B(target, 0x399) << 7;
            if ((int)((defAng - oppDir) & 0xFF) < 0x20) {
                return 0;
            }
        }
    }

    return 1;
}
