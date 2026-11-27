/*
 * func_80052720
 *
 * Purpose:
 *   Evaluates foul/card severity (0 = none/clean, 1 = foul/free kick,
 *   2 = yellow card, 3 = red card) for a challenge by `actor` against `other`.
 *   Skips when SCRATCH+0x2B0 == 4, match phase != 1, actor is GK (ID 1 or 12),
 *   or both players share the same attack direction (+0x399). Checks team card
 *   count in D_800FF748+0x88A5 (capping cards when >= 4 teammates booked or
 *   SCRATCH+0x2E1 == 3), computes relative approach angle vs (other+0x3AA - 8),
 *   dispatches bucket (angle_delta >> 4) via jtbl_80011DA4, and checks the
 *   attacking penalty area (signed_x >= 0x23E3, |z| < 0x12F4).
 *
 * Arguments:
 *   actor (unsigned char *): challenging player actor struct
 *   other (unsigned char *): challenged/ball-carrying player actor struct
 *
 * Return value:
 *   int: 0, 1, 2, or 3
 *
 * Scratchpad / Actor / Global fields read:
 *   SCRATCH 0x29A (u8,  R): match phase (1 = open play)
 *   SCRATCH 0x2B0 (u32, R): ball event code (4 disables check)
 *   SCRATCH 0x2E1 (u8,  R): match/game mode (3 suppresses extra cards)
 *   D_800FF748 + 0x88A5 + side*22 + slot (u8, R): player booking/card status
 *   Actor +0x02/+0x06 (s16, R): x, z position
 *   Actor +0x38D (u8, R): actor ID (1 and 12 excluded)
 *   Actor +0x392 (u8, R): roster slot
 *   Actor +0x398 (u8, R): side (0/1)
 *   Actor +0x399 (u8, R): attack direction flag (0/1)
 *   Other +0x02/+0x06 (s16, R): x, z position
 *   Other +0x399 (u8, R): attack direction flag
 *   Other +0x3AA (u8, R): body heading
 *
 * Magic numbers (250 raw = 1 m):
 *   0x23E3 (9187) = 36.748 m (penalty area x threshold)
 *   0x12F4 (4852) = 19.408 m (penalty area |z| threshold)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern void         *jtbl_80011DA4[];

extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_8001E6B8(unsigned int n);
extern unsigned int func_8001E730(unsigned int n);

int func_80052720(unsigned char *actor, unsigned char *other) {
    unsigned char id;
    unsigned int side;
    unsigned char *card_row;
    int booked_count;
    int i;
    int suppress_card;
    unsigned int ang;
    unsigned int ref_ang;
    unsigned int delta;
    unsigned int bucket;
    signed short signed_x;
    signed short abs_z;

    if (W(SCRATCH, 0x2B0) == 4u) {
        return 0;
    }
    if (B(SCRATCH, 0x29A) != 1) {
        return 0;
    }

    id = B(actor, 0x38D);
    if (id == 1 || id == 0x0C) {
        return 0;
    }
    if (B(actor, 0x399) == B(other, 0x399)) {
        return 0;
    }

    side = B(actor, 0x398);
    card_row = D_800FF748 + 0x88A5 + side * 22u;
    booked_count = 0;
    for (i = 0; i < 16; i++) {
        if (card_row[i] != 0) {
            booked_count++;
        }
    }

    if (booked_count >= 4 && D_800FF748[0x88A5 + B(actor, 0x398) * 22u + B(actor, 0x392)] == 0) {
        suppress_card = 1;
    } else {
        suppress_card = (B(SCRATCH, 0x2E1) == 3) ? 1 : 0;
    }

    ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(other, 0x02), S(other, 0x06));
    ref_ang = (unsigned int)B(other, 0x3AA) - 8u;
    if (((ang - ref_ang) & 0xFFu) < 0x81u) {
        ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(other, 0x02), S(other, 0x06));
        ref_ang = (unsigned int)B(other, 0x3AA) - 8u;
        delta = (ang - ref_ang) & 0xFFu;
    } else {
        ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(other, 0x02), S(other, 0x06));
        ref_ang = (unsigned int)B(other, 0x3AA) - 8u;
        delta = (ref_ang - ang) & 0xFFu;
    }

    bucket = delta >> 4;
    switch (bucket) {
    case 0:
        /* Block .L80052900: straight from behind */
        if (suppress_card != 0) {
            return 1;
        }
        if (func_8001E6B8(4) == 0 &&
            D_800FF748[0x88A5 + B(actor, 0x398) * 22u + B(actor, 0x392)] == 0) {
            return 3;
        }
        return 2;
    case 1:
    case 2: {
        /* Block .L8005295C: angled from behind */
        unsigned int r = func_8001E6B8(8);
        if (r == 0 || r == 2 || r == 6 || suppress_card != 0) {
            return 1;
        }
        return 2;
    }
    case 3:
    case 4:
    case 5:
        /* Block .L800529A8: side challenge */
        if ((func_8001E730(0x3C) & 0xFFu) != 0) {
            if (suppress_card != 0 || func_8001E6B8(0x0C) != 0) {
                return 1;
            }
            return 2;
        }
        goto penalty_box_check;
    case 6:
    case 7:
    case 8:
    default:
        /* Block .L800529E0: front challenge */
        if ((func_8001E730(0x32) & 0xFFu) != 0) {
            return 1;
        }
        break;
    }

penalty_box_check:
    signed_x = (B(actor, 0x399) != 0) ? (signed short)(-S(actor, 0x02)) : S(actor, 0x02);
    if (signed_x < 0x23E3) {
        return 0;
    }

    abs_z = S(actor, 0x06);
    if (abs_z < 0) {
        abs_z = (signed short)(-abs_z);
    }
    return (abs_z < 0x12F4) ? 1 : 0;
}
