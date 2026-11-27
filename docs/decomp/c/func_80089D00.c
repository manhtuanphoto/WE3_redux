/*
 * func_80089D00
 *
 * Purpose:
 *   Checks whether a pass receiver (specific actor targetId if != 0, or all 10
 *   teammates on the same team via func_8005CE14) is in an offside/targetable
 *   forward position relative to the passer (Scratchpad+0x2AA), the ball
 *   heading, and target (targetX, targetZ). On match (passing func_8008A144),
 *   publishes the free-kick spot (clamped to pitch bounds), copies Scratchpad
 *   0x1F0..0x20F -> 0x210..0x22F, increments team offside count at
 *   D_800FF748+0x8ECA+side, plays sound/event, and sets Scratchpad+0x294 = 7.
 *
 * Arguments:
 *   actor    - pointer to passing/last-owner 1000-byte actor struct
 *   targetId - specific candidate actor id (0 = scan 10 teammates)
 *   targetX  - reference x coordinate (s16)
 *   targetZ  - reference z coordinate (s16)
 *
 * Return value:
 *   None (void)
 *
 * Actor / global fields read:
 *   actor: +0x002 (s16) x, +0x006 (s16) z, +0x38D (u8) actor id,
 *          +0x399 (u8) attack direction flag
 *   candidate: +0x000 (u8) active flag, +0x002 (s16) x, +0x006 (s16) z,
 *              +0x38D (u8) actor id, +0x398 (u8) side
 *   D_800FF748 (24 x 1000-byte actor array)
 *   D_800FF748 + 0x8ECA + side (u8) per-side counter
 *
 * Actor / global fields written:
 *   D_800FF748 + 0x8ECA + side (u8) incremented by 1
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x1F0..+0x20C (8 x u32) saved offside-line/snapshot data
 *   +0x29A (u8) match phase (must be 1 = open play)
 *   +0x2AA (u8) previous ball owner (actor must match)
 *   +0x2E1 (u8) mode (must not be 5)
 *   +0x2F4 (ptr) ball actor (+0x3A4 move heading)
 *
 * Scratchpad fields written:
 *   +0x210..+0x22C (8 x u32) copied from +0x1F0..+0x20C
 *   +0x294 (u16) set to 7 (offside event)
 *   +0x2AD (u8) offending actor id
 *   +0x2AE (u8) result of func_8005CE30(offender id)
 *   +0x2BC (s16) spot x clamped to [-0x32A0, 0x32A0]
 *   +0x2BE (s16) spot z clamped to [-0x2080, 0x2080]
 *   +0x2F0 (u8) cleared to 0
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x40 (64) = 90 deg ball heading vs attack direction cone
 *   0x30 (48) = 67.5 deg angle cone from ball heading to candidate
 *   0x8FFFFF (9437183) -> sqrt ~ 3072 raw = 12.288 m max distance from (targetX, targetZ)
 *   0x32A0 (12960) = 51.84 m pitch half-length clamp for +0x2BC
 *   0x2080 (8320) = 33.28 m pitch half-width clamp for +0x2BE
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];

extern void func_800171C4(int arg0);
extern void func_8004F58C(int arg0);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8005CE14(int actorId);
extern int func_8005CE30(int actorId);
extern int func_8008A144(unsigned char *candidate);
extern void func_800AB620(int soundId);

void func_80089D00(unsigned char *actor, unsigned char targetId, short targetX, short targetZ)
{
    unsigned char *ball;
    int attackAngle;
    int ballHeading;
    int diff;
    int count;
    int i;
    unsigned char *cand;
    int angle;
    int dx, dz;

    if (B(Scratchpad, 0x29A) != 1) {
        return;
    }
    if (B(Scratchpad, 0x2E1) == 5) {
        return;
    }
    if (B(actor, 0x38D) != B(Scratchpad, 0x2AA)) {
        return;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    ballHeading = B(ball, 0x3A4);
    attackAngle = B(actor, 0x399) << 7;
    diff = (attackAngle - ballHeading) & 0xFF;
    if (diff < 0x81) {
        if (diff >= 0x40) {
            return;
        }
    } else {
        if (((ballHeading - attackAngle) & 0xFF) >= 0x40) {
            return;
        }
    }

    if (targetId != 0) {
        count = 1;
        cand = D_800FF748 + ((targetId % 24) & 0xFF) * 1000;
    } else {
        count = 10;
        cand = D_800FF748 + (func_8005CE14(B(actor, 0x38D)) & 0xFF) * 1000 + 0x3E8;
    }

    for (i = 0; i < count; i++, cand += 1000) {
        if (B(cand, 0) == 0 || cand == actor) {
            continue;
        }

        if (B(actor, 0x399) == 0) {
            if (S(cand, 2) < 0 || S(cand, 2) < S(actor, 2)) {
                continue;
            }
        } else {
            if (S(cand, 2) > 0 || S(actor, 2) < S(cand, 2)) {
                continue;
            }
        }

        angle = func_8005B36C(S(actor, 2), S(actor, 6), S(cand, 2), S(cand, 6));
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        if (((angle - B(ball, 0x3A4)) & 0xFF) < 0x81) {
            angle = func_8005B36C(S(actor, 2), S(actor, 6), S(cand, 2), S(cand, 6));
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            if (((angle - B(ball, 0x3A4)) & 0xFF) >= 0x30) {
                continue;
            }
        } else {
            angle = func_8005B36C(S(actor, 2), S(actor, 6), S(cand, 2), S(cand, 6));
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            if (((B(ball, 0x3A4) - angle) & 0xFF) >= 0x30) {
                continue;
            }
        }

        dx = S(cand, 2) - (int)targetX;
        dz = S(cand, 6) - (int)targetZ;
        if (dx * dx + dz * dz > 0x8FFFFF) {
            continue;
        }

        if ((func_8008A144(cand) & 0xFF) == 0) {
            continue;
        }

        func_800AB620(0x518);
        S(Scratchpad, 0x2BC) = S(cand, 2);
        S(Scratchpad, 0x2BE) = S(cand, 6);
        if (S(Scratchpad, 0x2BC) >= 0x32A1) {
            S(Scratchpad, 0x2BC) = 0x32A0;
        }
        if (S(Scratchpad, 0x2BC) < -0x32A0) {
            S(Scratchpad, 0x2BC) = -0x32A0;
        }
        if (S(Scratchpad, 0x2BE) >= 0x2081) {
            S(Scratchpad, 0x2BE) = 0x2080;
        }
        if (S(Scratchpad, 0x2BE) < -0x2080) {
            S(Scratchpad, 0x2BE) = -0x2080;
        }

        B(Scratchpad, 0x2AD) = B(cand, 0x38D);
        B(Scratchpad, 0x2AE) = (unsigned char)func_8005CE30(B(cand, 0x38D));

        W(Scratchpad, 0x210) = W(Scratchpad, 0x1F0);
        W(Scratchpad, 0x214) = W(Scratchpad, 0x1F4);
        W(Scratchpad, 0x218) = W(Scratchpad, 0x1F8);
        W(Scratchpad, 0x21C) = W(Scratchpad, 0x1FC);
        W(Scratchpad, 0x220) = W(Scratchpad, 0x200);
        W(Scratchpad, 0x224) = W(Scratchpad, 0x204);
        W(Scratchpad, 0x228) = W(Scratchpad, 0x208);
        W(Scratchpad, 0x22C) = W(Scratchpad, 0x20C);

        B(Scratchpad, 0x2F0) = 0;
        B(D_800FF748, 0x8ECA + B(cand, 0x398)) += 1;
        func_8004F58C(3);
        func_800171C4(0xC);
        H(Scratchpad, 0x294) = 7;
        return;
    }
}
