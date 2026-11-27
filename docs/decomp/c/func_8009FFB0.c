/*
 * func_8009FFB0
 *
 * Purpose:
 *   Open-play tactical/AI update (runs only when Scratchpad+0x29A == 1):
 *   1) Advances timer Scratchpad+0x4, setting +0xA = 1 at 17 frames and
 *      +0x9 = 1 (resetting +0x4 to 0x40) at 33 frames; decrements +0xB.
 *   2) Decrements D_8010C1E7; when 0, checks whether >= 2 outfield players on
 *      the opposing team lie within +-0x1C (39.375 deg) of the ball's heading
 *      (+0x3A8), updating D_8010C1D8[0], D_8010C1D8[0xF], and Scratchpad+0xB.
 *   3) Clears D_8010C1D8[0xC], and if the ball owner (Scratchpad+0x2A9) has
 *      role/slot (+0x391) >= 0x11, searches the 10 outfield players of the
 *      selected team for a matching role/side partner (+0x391 in 0x11..0x19)
 *      to record in D_8010C1D8[0xC].
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Actor / global fields read:
 *   D_800FF748 (actor array, outfield players at +0x7D0 [idx 2] or +0x32C8 [idx 13], stride 1000):
 *     +0x000 (u8) active flag, +0x002 (s16) x, +0x006 (s16) z,
 *     +0x38D (u8) actor id, +0x391 (u8) tactical role/slot
 *   D_800FF748 + 0xB68 (u8) actor 2's +0x398 side
 *   D_801002B0 (u8) team selector index
 *   D_8010C1E7 (u8) cooldown timer
 *
 * Global fields written:
 *   D_8010C1D8[0x00] (u8) clear/open-lane flag (0 or 1)
 *   D_8010C1D8[0x0C] (u8) matched partner actor id (0 if none)
 *   D_8010C1D8[0x0F] (u8) set to 0x10 when D_8010C1D8[0] == 1
 *   D_8010C1E7 (u8) decremented by 1 when nonzero
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x004 (u8) timer, +0x00B (u8) cooldown, +0x29A (u8) match phase (1=open play),
 *   +0x2A9 (u8) ball owner id, +0x2CF[2] (u8) possession flags,
 *   +0x2F4 (ptr) ball actor (+0x002 x, +0x006 z, +0x3A8 heading),
 *   +0x30B (u8) mode/state (4 checked)
 *
 * Scratchpad fields written:
 *   +0x004 (u8), +0x009 (u8), +0x00A (u8), +0x00B (u8)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x11 (17 frames = 0.567 s), 0x21 (33 frames = 1.10 s), 0x40 (64)
 *   0x1C (28) = 39.375 deg half-cone around ball+0x3A8
 *   0x10 (16 frames = 0.533 s) cooldown set on +0xB and D_8010C1D8[0xF]
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_801002B0;
extern unsigned char D_8010C1D8[];
extern unsigned char D_8010C1E7;

extern int func_8005B36C(int x1, int z1, int x2, int z2);

void func_8009FFB0(void)
{
    unsigned int t;
    int count;
    int i;
    unsigned char *cand;
    unsigned char *ball;
    unsigned char *owner;
    int angle;
    int diff;

    if (B(Scratchpad, 0x29A) != 1) {
        return;
    }

    t = (B(Scratchpad, 4) + 1) & 0xFF;
    B(Scratchpad, 4) = (unsigned char)t;
    if (t >= 0x11) {
        B(Scratchpad, 0xA) = 1;
    }
    if (t >= 0x21) {
        B(Scratchpad, 9) = 1;
        B(Scratchpad, 4) = 0x40;
    }

    if (B(Scratchpad, 0xB) != 0) {
        B(Scratchpad, 0xB) -= 1;
    }

    if (D_8010C1E7 != 0) {
        D_8010C1E7 -= 1;
    } else if (B(Scratchpad, 0x30B) == 4) {
        B(D_8010C1D8, 0) = 0;
        B(Scratchpad, 0xB) = 0;
    } else {
        count = 0;
        if (B(Scratchpad, 0x2CF + D_801002B0) != 0) {
            cand = D_800FF748 + 0x7D0;
        } else {
            cand = D_800FF748 + 0x32C8;
        }
        B(D_8010C1D8, 0) = 1;

        for (i = 0; (i & 0xFF) < 0xA; i++, cand += 1000) {
            if (B(cand, 0) == 0) {
                continue;
            }
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            angle = func_8005B36C(S(ball, 2), S(ball, 6), S(cand, 2), S(cand, 6));
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            if (((angle - B(ball, 0x3A8)) & 0xFF) < 0x81) {
                angle = func_8005B36C(S(ball, 2), S(ball, 6), S(cand, 2), S(cand, 6));
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                diff = (angle - B(ball, 0x3A8)) & 0xFF;
            } else {
                angle = func_8005B36C(S(ball, 2), S(ball, 6), S(cand, 2), S(cand, 6));
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                diff = (B(ball, 0x3A8) - angle) & 0xFF;
            }
            if (diff < 0x1C) {
                B(D_8010C1D8, 0) = 0;
                count++;
            }
        }

        if ((count & 0xFF) < 2) {
            B(Scratchpad, 0xB) = 0x10;
        }
        if (B(D_8010C1D8, 0) == 1) {
            B(D_8010C1D8, 0xF) = 0x10;
        }
    }

    B(D_8010C1D8, 0xC) = 0;
    if (B(Scratchpad, 0x2A9) == 0) {
        return;
    }

    owner = D_800FF748 + ((B(Scratchpad, 0x2A9) % 24) & 0xFF) * 1000;
    if (B(owner, 0x391) < 0x11) {
        return;
    }

    if (B(Scratchpad, 0x2CF + B(D_800FF748, 0xB68)) == 1) {
        cand = D_800FF748 + 0x7D0;
    } else {
        cand = D_800FF748 + 0x32C8;
    }

    for (i = 0; (i & 0xFF) < 0xA; i++, cand += 1000) {
        unsigned char candRole;
        unsigned char ownerRole;

        if (B(cand, 0) == 0) {
            continue;
        }
        if (B(cand, 0x38D) == B(Scratchpad, 0x2A9)) {
            continue;
        }

        candRole = B(cand, 0x391);
        if ((unsigned int)(candRole - 0x11) >= 9) {
            continue;
        }

        ownerRole = B(owner, 0x391);
        switch (candRole) {
        case 0x11:
        case 0x12:
            if (ownerRole >= 0x17 && ownerRole < 0x1A) {
                int ballSideZ = (S((unsigned char *)W(Scratchpad, 0x2F4), 6) >= 0) ? 1 : -1;
                int candSideZ = (S(cand, 6) >= 0) ? 1 : -1;
                if (ballSideZ == candSideZ) {
                    B(D_8010C1D8, 0xC) = B(cand, 0x38D);
                    return;
                }
            }
            break;

        case 0x13:
        case 0x15:
            if (ownerRole == 0x11 || ownerRole == 0x13 || ownerRole == 0x16) {
                B(D_8010C1D8, 0xC) = B(cand, 0x38D);
                return;
            }
            break;

        case 0x14:
        case 0x16:
            if (ownerRole == 0x12 || ownerRole == 0x14 || ownerRole == 0x15) {
                B(D_8010C1D8, 0xC) = B(cand, 0x38D);
                return;
            }
            break;

        case 0x17:
        case 0x18:
        case 0x19:
            B(D_8010C1D8, 0xC) = B(cand, 0x38D);
            return;
        }
    }
}
