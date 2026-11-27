/*
 * func_80035320 - Assigns pressing/marking/shot-reaction roles to teammates and defenders.
 *
 * Purpose:
 *   Updates spatial pressure assignments (func_800676C4) and dispatches nearby
 *   teammates/opponents into pressing/chasing (state 0x1B) or goalkeeper save/brace
 *   (state 0x1A) depending on the actor's action type (+0x3C4: 2/6 vs shot/other)
 *   and the predicted ball landing coordinates.
 *
 * Arguments:
 *   actor (unsigned char*): initiating player actor struct (1000 bytes).
 *   a1 (int): target X coordinate (used as s16 when +0x3C4 == 2 or 6).
 *   a2 (int): target Z coordinate (used as s16 when +0x3C4 == 2 or 6).
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F8000F2 (s16, W): reference Y height (-0x280 or -0x200).
 *   - 0x1F8000F8 / 0x1F8000FC (s16, R): predicted ball landing X/Z.
 *   - 0x1F8000FE / 0x1F800100 (s16, W): stored target X/Z when +0x3C4 == 2 or 6.
 *   - 0x1F80029A (u8, R): match phase (1 = open play).
 *   - 0x1F8002B8 / 0x1F8002BA (s16, R): fallback ball target X/Z when |ball.x| >= 0x3300.
 *   - 0x1F8002E1 (u8, R): match mode (5 skips opponent dispatch).
 *   - 0x1F8002F4 (u8*, R): pointer to ball actor.
 *   - 0x1F80031C (s16, W): ball timer (set to (atk_dir ^ 1) + 8 frames when +0x3C4 == 2 or 6).
 *   - 0x1F80031E[2][2] (u8, R/W): assigned presser actor IDs per attack direction.
 *   - 0x1F800322[2][2] (u8, R): nearest candidate actor IDs per attack direction.
 *
 * Actor fields reads/writes:
 *   - actor: +0x399 (u8 attack dir, R), +0x3C4 (s16 action/shot subtype, R).
 *   - ball:  +0x02 (s16 x, R), +0x3BC (s16, W=1).
 *   - target (D_800FF748 + (id % 24) * 1000):
 *            +0x02/+0x06 (s16 x/z, R), +0x38D (u8 id, R), +0x396 (u8 state, W=0x1B/0x1A),
 *            +0x397 (u8 substate, W=0), +0x3C2 (s16, W=0x10), +0x3C6 (s16 lateral target, W).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - -0x280 (-640) = -2.56 m, -0x200 (-512) = -2.048 m reference Y height.
 *   - 0x3300 (13056) = 52.22 m, 0x3320 (13088) = 52.35 m, 0x3420 (13344) = 53.38 m goal-line X.
 *   - 0x8FFFFF (9437183, ~3072^2) = 12.29 m squared goal-distance threshold for GK state 0x1A.
 *   - 0x801 (2049) = 8.20 m |Z - tz| goal-width bound for GK state 0x1A.
 *   - 0x260 (608) = 2.43 m +/- 0x80 (128 = 0.51 m) lateral offset stored in target +0x3C6.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];

extern void func_800676C4(int atk_dir, int x, int z, int a3);
extern int func_80049D94(unsigned char *actor);
extern int func_80052C6C(unsigned char *actor);
extern int func_80052D90(unsigned char *actor);
extern void func_800512B8(unsigned char *actor, int a1);
extern void func_800489A8(void);
extern int func_8001E6B8(int n);
extern int func_8005C7A0(int n);

void func_80035320(unsigned char *actor, int a1, int a2)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *base = D_800FF748;
    unsigned char *ball;
    unsigned char *target;
    int tx, tz;
    int idx;
    int i;
    int c4;
    int opp_dir;
    int goal_x;
    int dx, dz, abs_dz;
    int val;

    c4 = S(actor, 0x3C4);
    if (c4 == 2 || c4 == 6) {
        tx = (short)a1;
        tz = (short)a2;
        S(scratch, 0xFE) = (short)a1;
        S(scratch, 0x100) = (short)a2;
        S(scratch, 0x31C) = (short)((B(actor, 0x399) ^ 1) + 8);
        func_800676C4(B(actor, 0x399), tx, tz, 0);

        idx = func_80049D94(actor) & 0xFF;
        target = base + ((unsigned int)idx % 24U) * 1000;
        if ((func_80052C6C(target) & 0xFF) == 0) {
            func_800512B8(target, 1);
            B(target, 0x396) = 0x1B;
            B(target, 0x397) = 0;
            S(target, 0x3C2) = 0x10;
            B(scratch, 0x31E + B(actor, 0x399) * 2) = B(target, 0x38D);
        }

        func_800676C4(B(actor, 0x399) ^ 1, tx, tz, 0);
        for (i = 0; i < 2; i++) {
            idx = B(scratch, 0x322 + (B(actor, 0x399) ^ 1) * 2 + i);
            target = base + ((unsigned int)idx % 24U) * 1000;
            if ((func_80052C6C(target) & 0xFF) == 0) {
                B(target, 0x396) = 0x1B;
                B(target, 0x397) = 0;
                S(target, 0x3C2) = 0x10;
                if (i == 0) {
                    func_800512B8(target, 1);
                }
                B(scratch, 0x31E + (B(actor, 0x399) ^ 1) * 2 + i) = B(target, 0x38D);
            }
        }
        return;
    }

    ball = (unsigned char *)W(scratch, 0x2F4);
    S(ball, 0x3BC) = 1;
    c4 = S(actor, 0x3C4);
    if (c4 == 0) {
        S(scratch, 0xF2) = -0x280;
    } else {
        S(scratch, 0xF2) = -0x200;
    }

    func_800489A8();
    ball = (unsigned char *)W(scratch, 0x2F4);
    tx = S(scratch, 0xF8);
    tz = S(scratch, 0xFC);
    if (S(ball, 2) <= -0x3300 || S(ball, 2) >= 0x3300) {
        tx = S(scratch, 0x2B8);
        tz = S(scratch, 0x2BA);
    }

    func_800676C4(B(actor, 0x399), tx, tz, 0);
    idx = func_80049D94(actor) & 0xFF;
    target = base + ((unsigned int)idx % 24U) * 1000;
    if ((func_80052D90(target) & 0xFF) == 0) {
        func_800512B8(target, 1);
        B(target, 0x396) = 0x1B;
        B(target, 0x397) = 0;
        S(target, 0x3C2) = 0x10;
        B(scratch, 0x31E + B(actor, 0x399) * 2) = B(target, 0x38D);
    }

    if (B(scratch, 0x2E1) == 5) {
        return;
    }

    opp_dir = B(actor, 0x399) ^ 1;
    func_800676C4(opp_dir, tx, tz, 0);
    opp_dir = B(actor, 0x399) ^ 1;
    B(scratch, 0x31E + opp_dir * 2) = B(scratch, 0x322 + opp_dir * 2);
    B(scratch, 0x31F + opp_dir * 2) = B(scratch, 0x323 + opp_dir * 2);

    idx = B(scratch, 0x322 + (B(actor, 0x399) ^ 1) * 2);
    target = base + ((unsigned int)idx % 24U) * 1000;
    func_800512B8(target, 1);
    if ((func_80052D90(target) & 0xFF) == 0) {
        B(target, 0x396) = 0x1B;
        B(target, 0x397) = 0;
        S(target, 0x3C2) = 0x10;
    }

    if (B(scratch, 0x29A) == 1) {
        if (func_8001E6B8(4) != 0 || S(actor, 0x3C4) != 0) {
            return;
        }
    }

    if (B(actor, 0x399) != 0) {
        goal_x = -0x3420;
    } else {
        goal_x = 0x3420;
    }
    func_800676C4(B(actor, 0x399) ^ 1, goal_x, 0, 0);

    opp_dir = B(actor, 0x399) ^ 1;
    if (B(scratch, 0x31E + opp_dir * 2) != B(scratch, 0x322 + opp_dir * 2)) {
        idx = B(scratch, 0x322 + opp_dir * 2);
    } else {
        idx = B(scratch, 0x323 + opp_dir * 2);
    }
    target = base + ((unsigned int)idx % 24U) * 1000;

    if (B(actor, 0x399) != 0) {
        dx = S(target, 2) + 0x3320;
    } else {
        dx = S(target, 2) - 0x3320;
    }
    dz = S(target, 6) - tz;
    if (dx * dx + dz * dz > 0x8FFFFF) {
        return;
    }

    abs_dz = S(target, 6) - tz;
    if (abs_dz < 0) {
        abs_dz = -abs_dz;
    }
    if (abs_dz >= 0x801) {
        return;
    }

    if ((func_80052D90(target) & 0xFF) != 0) {
        return;
    }

    val = (short)func_8005C7A0(0x80) + 0x260;
    if (S(target, 6) < 0) {
        val = -val;
    }
    S(target, 0x3C6) = (short)val;
    B(target, 0x396) = 0x1A;
    B(target, 0x397) = 0;
}
