/*
 * func_8003C0E8 - Player kick / shot / clearance action handler
 *
 * Purpose:
 *   Executes a player's grounded kick action (sub-state +0x397: 0 = wind-up &
 *   ball release, nonzero = follow-through until anim frame 0x1C). Turns the
 *   player toward target heading (+0x3AB), advances position by (+0x3CA, +0x3CE),
 *   and at anim frame >= 7 checks ball contact via func_8003D274. If behind
 *   signed X < 0x1990 (26.208 m), performs a clearance/long kick; otherwise
 *   computes shot power and lift from player stats (D_8010C32C, D_8010C330
 *   weak-foot 16/17 penalty) and fires a shot via func_8004B0F0 + func_8002B5A4.
 *
 * Arguments:
 *   actor (unsigned char *): pointer to 1000-byte player actor struct.
 *
 * Return value:
 *   void.
 *
 * Scratchpad (0x1F800000) reads/writes:
 *   - 0x2B4 / 0x2B6 (s16, R): ball target/landing X, Z passed to func_80049A94.
 *   - 0x2F4 (u8 *, R): pointer to ball actor (+0x396, +0x3B2, +0x3C8, +0x3CA).
 *   - 0x31C (s16, W): set to attack_dir ^ 1 on clearance kick.
 *
 * Actor fields read/written:
 *   - +0x02/+0x06 (s16, R/W): X/Z position.
 *   - +0x26 (u16, W): model Y rotation = ((0x1C0 - heading) % 256) << 4.
 *   - +0x390 (u8, R): animation frame counter.
 *   - +0x392 (u8, R): team slot (0..10, stride 12 in D_8010C32C/30).
 *   - +0x396 (u8, W): actor state (cleared to 0 in follow-through).
 *   - +0x397 (u8, R/W): action sub-state (0 -> 1 at contact; cleared at frame 0x1C).
 *   - +0x398 (u8, R): side (0/1, stride 264 in D_8010C32C/30).
 *   - +0x399 (u8, R): attack direction flag (0/1).
 *   - +0x3AA (u8, R/W): body heading.
 *   - +0x3AB (u8, R): target heading.
 *   - +0x3AC (u8, R): kicking foot flag (compared with D_8010C330 bit 4).
 *   - +0x3B1 (u8, R): ball spin/curve value copied to ball+0x3CA on shot.
 *   - +0x3B2 (u8, R): shot/kick aim target parameter passed to func_8003D99C.
 *   - +0x3CA/+0x3CE (s16, R): X/Z per-frame step velocity.
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x1990 (6544 raw = 26.176 m): X threshold separating clearance vs shot on goal.
 *   - 0x507: kick sound effect ID (func_800AB620).
 *   - 0x1C (28 frames): follow-through completion frame.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_8010C32C[];
extern unsigned char D_8010C330[];

extern void func_8005BD1C(unsigned char *actor);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int current, int step);
extern unsigned int func_8003D274(unsigned char *actor, int arg1, int arg2);
extern int func_8005C7A0(int range);
extern int func_8001E6B8(int range);
extern int func_8002B1E4(unsigned char *actor);
extern unsigned int func_8003D99C(unsigned char *actor, unsigned int arg1, int arg2);
extern void func_8004B0F0(unsigned char *actor, unsigned int dir, int speed, int lift);
extern void func_800AB620(int sfx_id);
extern void func_8004B278(unsigned char *actor);
extern void func_80049A94(unsigned char *actor, unsigned int mode, unsigned int flag2, int x, int z);
extern void func_8002B5A4(unsigned char *actor, unsigned int arg1);
extern void func_8005707C(unsigned char *actor);

void func_8003C0E8(unsigned char *actor)
{
    unsigned int heading;
    int rot_diff;
    signed short signed_x;
    unsigned char *ball;

    func_8005BD1C(actor);

    if (B(actor, 0x397) != 0) {
        B(actor, 0x396) = 0;
        if (B(actor, 0x390) == 0x1C) {
            B(actor, 0x397) = 0;
        }
        return;
    }

    heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0x28);
    B(actor, 0x3AA) = (unsigned char)heading;

    rot_diff = 0x1C0 - (int)(heading & 0xFFu);
    H(actor, 0x26) = (unsigned short)((rot_diff % 256) << 4);

    H(actor, 0x2) = (unsigned short)(H(actor, 0x2) + H(actor, 0x3CA));
    H(actor, 0x6) = (unsigned short)(H(actor, 0x6) + H(actor, 0x3CE));

    if (B(actor, 0x390) < 7) {
        return;
    }

    B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
    if ((func_8003D274(actor, 1, 1) & 0xFFu) == 0u) {
        return;
    }

    signed_x = S(actor, 0x2);
    if (B(actor, 0x399) != 0) {
        signed_x = (signed short)(-signed_x);
    }

    if (signed_x < 0x1990) {
        int rnd_ang = func_8005C7A0(8);
        unsigned int kick_dir = func_8005B7CC(
            ((B(actor, 0x399) << 7) + rnd_ang) & 0xFFu,
            B(actor, 0x3AA),
            0x24
        );
        int rnd_spd1 = (signed short)func_8005C7A0(8);
        int rnd_spd2 = func_8001E6B8(8) + 0x70;
        int lift = func_8001E6B8(4) + 0x1C;

        func_8004B0F0(actor, kick_dir & 0xFFu, rnd_spd1 + rnd_spd2, lift);
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        B(ball, 0x3B2) = 8;
        B(ball, 0x396) = 4;
        H(Scratchpad, 0x31C) = (unsigned short)((B(actor, 0x399) ^ 1u) & 0xFFu);
        func_800AB620(0x507);
        func_8004B278(actor);
        func_80049A94(actor, 1, 1, S(Scratchpad, 0x2B4), S(Scratchpad, 0x2B6));
    } else {
        int power;
        int lift;
        unsigned int pref_foot;
        unsigned int shot_dir;
        int spin_rand;

        if (func_8001E6B8(3) != 0) {
            power = func_8001E6B8(4) + 0xB0;
            if (func_8001E6B8(3) != 0) {
                unsigned int tbl_off = B(actor, 0x392) * 12u + B(actor, 0x398) * 264u;
                unsigned int stat = (W(D_8010C32C, tbl_off) >> 4) & 0xFu;
                int r = func_8001E6B8(0xD - (int)stat);
                lift = r + func_8002B1E4(actor) + 0xA;
            } else {
                lift = func_8001E6B8(4) + 0x4;
            }
        } else {
            power = func_8001E6B8(4) + 0xB8;
            lift = func_8001E6B8(4) - 0xF;
        }

        {
            unsigned int tbl_off = B(actor, 0x392) * 12u + B(actor, 0x398) * 264u;
            pref_foot = (W(D_8010C330, tbl_off) >> 4) & 1u;
        }
        if (B(actor, 0x3AC) != pref_foot) {
            /* Weak-foot power scaling: * 16 / 17 */
            power = (int)(((unsigned long long)((unsigned int)(power << 8)) * 0xF0F0F0F1ULL) >> 40);
        }

        shot_dir = func_8003D99C(actor, B(actor, 0x3AB), 1);
        func_8004B0F0(actor, shot_dir & 0xFFu, power, lift);

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        H(ball, 0x3CA) = B(actor, 0x3B1);
        spin_rand = func_8001E6B8(3);
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        H(ball, 0x3C8) = (unsigned short)(spin_rand + 5);
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        B(ball, 0x396) = 4;
        func_800AB620(0x507);
        func_8002B5A4(actor, 2);
        func_8005707C(actor);
    }
}
