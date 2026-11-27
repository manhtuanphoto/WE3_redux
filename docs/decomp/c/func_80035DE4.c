/*
 * func_80035DE4
 *
 * Purpose:
 *   Per-frame state machine (substate +0x397 = 0..3) for a jumping/airborne
 *   kick/header/volley action:
 *     0: turns toward +0x3AB at step 0x10 and advances to substate 1.
 *     1: turns toward +0x3AB at step 0x0C, advances xyz by (+0x3CA, +0x3CC,
 *        +0x3CE), and on anim frame 8 checks ball contact via func_8003D274;
 *        if hit, computes shot/header direction via func_80036230 and power/lift
 *        from player stat, facing diff, +0x39B, and lift, then kicks via
 *        func_8004B0F0 and sets ball contact/state; scales +0x3CC by 3/2 and
 *        advances to substate 2.
 *     2: turns at step 8, integrates x/z and subtracts +0x3CC from y, damps
 *        +0x3CA/+0x3CE by 9/10, and at anim frame >= 15 grounds y=0 and either
 *        triggers func_800363B8 or advances to substate 3.
 *     3: waits until anim frame >= 24, then resets state (+0x396) and substate
 *        (+0x397) to 0.
 *
 * Arguments:
 *   actor - pointer to 1000-byte actor struct
 *
 * Return value:
 *   None (void)
 *
 * Actor / global fields read:
 *   +0x002 (s16) x, +0x004 (s16) y, +0x006 (s16) z,
 *   +0x390 (u8) anim frame, +0x392 (u8) team slot, +0x397 (u8) substate,
 *   +0x398 (u8) side, +0x39B (u8) pressure/flag,
 *   +0x3A4 (u8) move heading, +0x3AA (u8) body heading, +0x3AB (u8) target heading,
 *   +0x3B2 (u8) saved heading, +0x3CA (s16) step dx, +0x3CC (s16) step dy,
 *   +0x3CE (s16) step dz
 *   D_8010C32C[team_slot*12 + side*264] (u32) bits 20..23 (player stat 0..15)
 *
 * Actor / global fields written:
 *   +0x002 (s16) x, +0x004 (s16) y, +0x006 (s16) z, +0x026 (s16) rotation,
 *   +0x396 (u8) state, +0x397 (u8) substate, +0x3AA (u8) body heading,
 *   +0x3AB (u8) target heading, +0x3CA/+0x3CC/+0x3CE (s16) step velocities,
 *   +0x3D1 (u8) set to 3 on missed contact at frame 8
 *
 * Scratchpad (0x1F800000) fields read / written:
 *   +0x2F4 (ptr) ball actor (+0x396 state = 4, +0x3C8 = 12 - rand(3), +0x3CA = rand(2))
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   frame 8 (0.267 s) = contact/kick frame
 *   frame 15 (0.50 s) = landing frame
 *   frame 24 (0.80 s) = recovery completion frame
 *   0xA8 (168) = base kick power before stat/angle/lift penalties
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_8010C32C[];

extern int func_8001E6B8(int n);
extern void func_8002B5A4(unsigned char *actor, int arg1);
extern int func_80036230(unsigned char *actor);
extern void func_800363B8(unsigned char *actor);
extern int func_8003D274(unsigned char *actor, int a1, int a2);
extern void func_8003D99C(unsigned char *actor, int heading, int a2);
extern void func_8004B0F0(unsigned char *actor, int heading, int power, int lift);
extern void func_8005715C(unsigned char *actor);
extern int func_8005B7CC(int target, int cur, int step);
extern void func_8005BD1C(unsigned char *actor);
extern void func_800AB620(int soundId);

void func_80035DE4(unsigned char *actor)
{
    int heading;
    int turn;
    int s5; /* UNCLEAR: only initialized on func_8003D274 != 0 path in original asm */
    int lift;
    int row;
    int stat;
    int power;
    int diff;
    unsigned char *ball;

    switch (B(actor, 0x397)) {
    case 0:
        func_8005BD1C(actor);
        heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0x10);
        B(actor, 0x3AA) = (unsigned char)heading;
        turn = 0x1C0 - (heading & 0xFF);
        S(actor, 0x26) = (signed short)((turn % 256) << 4);
        B(actor, 0x397) += 1;
        break;

    case 1:
        func_8005BD1C(actor);
        heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
        B(actor, 0x3AA) = (unsigned char)heading;
        turn = 0x1C0 - (heading & 0xFF);
        S(actor, 0x26) = (signed short)((turn % 256) << 4);

        S(actor, 2) = (signed short)(H(actor, 2) + S(actor, 0x3CA));
        S(actor, 6) = (signed short)(H(actor, 6) + S(actor, 0x3CE));
        S(actor, 4) = (signed short)(H(actor, 4) + S(actor, 0x3CC));

        if (B(actor, 0x390) == 8) {
            if ((func_8003D274(actor, 1, 1) & 0xFF) != 0) {
                func_8003D99C(actor, B(actor, 0x3B2), 0);
                s5 = func_80036230(actor);
                lift = s5;

                row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
                stat = (W(D_8010C32C, row) >> 20) & 0xF;
                power = 0xA8 - func_8001E6B8(9 - stat);

                diff = (B(actor, 0x3A4) - B(actor, 0x3AB)) & 0xFF;
                if (diff < 0x81) {
                    power -= diff >> 3;
                } else {
                    power -= ((B(actor, 0x3AB) - B(actor, 0x3A4)) & 0xFF) >> 3;
                }

                if (B(actor, 0x39B) != 0) {
                    power -= func_8001E6B8(0x10);
                }

                if ((signed short)lift > 0) {
                    if (func_8001E6B8(2) != 0) {
                        power -= lift;
                    } else {
                        power -= (signed short)lift >> 1;
                    }
                }

                func_8004B0F0(actor, s5 & 0xFF, power & 0xFF, (signed short)lift);

                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                S(ball, 0x3CA) = (signed short)func_8001E6B8(2);
                diff = func_8001E6B8(3);
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                S(ball, 0x3C8) = (signed short)(0xC - diff);
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                B(ball, 0x396) = 4;

                func_800AB620(0x507);
                func_8002B5A4(actor, 0);
                func_8005715C(actor);
            } else {
                B(actor, 0x3D1) = 3;
            }
            B(actor, 0x3AB) = (unsigned char)s5;
            S(actor, 0x3CC) = (signed short)((S(actor, 0x3CC) * 3) / 2);
            B(actor, 0x397) += 1;
        }
        break;

    case 2:
        func_8005BD1C(actor);
        heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 8);
        B(actor, 0x3AA) = (unsigned char)heading;
        turn = 0x1C0 - (heading & 0xFF);
        S(actor, 0x26) = (signed short)((turn % 256) << 4);

        S(actor, 2) = (signed short)(H(actor, 2) + S(actor, 0x3CA));
        S(actor, 4) = (signed short)(H(actor, 4) - S(actor, 0x3CC));
        S(actor, 6) = (signed short)(H(actor, 6) + S(actor, 0x3CE));

        S(actor, 0x3CA) = (signed short)((S(actor, 0x3CA) * 9) / 10);
        S(actor, 0x3CE) = (signed short)((S(actor, 0x3CE) * 9) / 10);

        if (B(actor, 0x390) >= 0xF) {
            S(actor, 4) = 0;
            if (B(actor, 0x39B) != 0 && func_8001E6B8(3) == 0) {
                func_800363B8(actor);
            } else {
                B(actor, 0x397) += 1;
            }
        }
        break;

    case 3:
        func_8005BD1C(actor);
        if (B(actor, 0x390) >= 0x18) {
            B(actor, 0x396) = 0;
            B(actor, 0x397) = 0;
        }
        break;
    }
}
