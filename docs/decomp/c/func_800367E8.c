/*
 * func_800367E8
 * Purpose: Acrobatic / airborne volley or diving kick state handler.
 *          Turns actor toward target heading (substates 0..2), integrates 3D jump velocity
 *          (+0x3CA, +0x3CC, +0x3CE), checks ball contact at anim frame 8 to kick with
 *          func_8004B0F0, decays height/velocity until frame 15, and transitions to recovery
 *          state 0x22 substate 3 (anim 0x24).
 * Arguments:
 *   actor - pointer to 1000-byte player actor struct
 * Return value:
 *   void
 * Scratchpad reads/writes:
 *   R: 0x2F4 (ball actor ptr)
 *   W: none directly
 * Actor reads/writes:
 *   R: +0x02/+0x04/+0x06 (x/y/z), +0x390 (anim frame), +0x392 (team slot), +0x397 (substate),
 *      +0x398 (side), +0x39B (flag), +0x3AA (body heading), +0x3AB, +0x3B0,
 *      +0x3CA/+0x3CC/+0x3CE (jump velocity x/y/z)
 *   W: +0x02/+0x04/+0x06 (x/y/z), +0x26 (rotation), +0x396 (state = 0x22), +0x397 (substate),
 *      +0x3AA (body heading), +0x3CA/+0x3CC/+0x3CE (decayed velocity), +0x3D1 (= 3 on miss)
 *   Ball actor W: +0x396 (= 4), +0x3C8, +0x3CA
 * Constants:
 *   0xA4 (164) base kick power; frame 8 contact check; frame 15 (0x0F = 0.5 s) landing.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCH  ((unsigned char *)0x1F800000)

extern unsigned char D_8010C32C[];

extern int func_8001E6B8(int n);
extern void func_8002B5A4(unsigned char *actor, int arg1);
extern int func_80036230(unsigned char *actor);
extern void func_800363B8(unsigned char *actor);
extern unsigned int func_8003D274(unsigned char *actor, int a1, int a2);
extern void func_8003D99C(unsigned char *actor, unsigned int a1, int a2);
extern void func_8003DB08(unsigned char *actor);
extern void func_8004B0F0(unsigned char *actor, unsigned int heading, unsigned int power, int lift);
extern void func_8005715C(unsigned char *actor);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005BD1C(unsigned char *actor);
extern void func_800AB620(int sfx);

void func_800367E8(unsigned char *actor) {
    unsigned char *ball;
    unsigned int heading;
    int turn;
    int kick_res;
    int lift;
    int stat;
    int power;
    unsigned int diff;
    int penalty;

    switch (B(actor, 0x397)) {
    case 0:
        func_8005BD1C(actor);
        heading = func_8005B7CC(B(actor, 0x3B0), B(actor, 0x3AA), 0xC) & 0xFF;
        B(actor, 0x3AA) = heading;
        turn = 0x1C0 - (int)heading;
        H(actor, 0x26) = (unsigned short)((turn % 256) << 4);
        if (B(actor, 0x390) != 0) {
            B(actor, 0x397) = B(actor, 0x397) + 1;
        }
        break;

    case 1:
        func_8005BD1C(actor);
        heading = func_8005B7CC(B(actor, 0x3B0), B(actor, 0x3AA), 0xC) & 0xFF;
        B(actor, 0x3AA) = heading;
        turn = 0x1C0 - (int)heading;
        H(actor, 0x26) = (unsigned short)((turn % 256) << 4);

        S(actor, 2) = S(actor, 2) + S(actor, 0x3CA);
        S(actor, 4) = S(actor, 4) + S(actor, 0x3CC);
        S(actor, 6) = S(actor, 6) + S(actor, 0x3CE);

        if (B(actor, 0x390) < 8) {
            break;
        }

        if ((func_8003D274(actor, 1, 1) & 0xFF) != 0) {
            func_8003D99C(actor, B(actor, 0x3AB), 0);
            kick_res = func_80036230(actor);
            lift = (signed short)kick_res;

            stat = (W(D_8010C32C, B(actor, 0x398) * 0x108 + B(actor, 0x392) * 0xC) >> 20) & 0xF;
            power = 0xA4 - func_8001E6B8(9 - stat);

            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            diff = (unsigned int)((kick_res - B(ball, 0x3A4)) - 0x80);
            if ((diff & 0xFF) < 0x81) {
                penalty = (int)(diff & 0xFF) >> 3;
            } else {
                penalty = (int)((-(int)diff) & 0xFF) >> 3;
            }
            power -= penalty;

            if (B(actor, 0x39B) != 0) {
                power -= func_8001E6B8(0x10);
            }

            if (lift > 0) {
                if (func_8001E6B8(2) != 0) {
                    power -= lift;
                } else {
                    power -= lift >> 1;
                }
            }

            func_8004B0F0(actor, kick_res & 0xFF, power & 0xFF, lift);

            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            S(ball, 0x3CA) = (signed short)func_8001E6B8(2);
            S(ball, 0x3C8) = (signed short)(0xC - func_8001E6B8(3));
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            B(ball, 0x396) = 4;

            func_800AB620(0x507);
            func_8002B5A4(actor, 0);
            func_8005715C(actor);
        } else {
            B(actor, 0x3D1) = 3;
        }

        S(actor, 0x3CC) = (signed short)((S(actor, 0x3CC) * 3) / 2);
        B(actor, 0x397) = B(actor, 0x397) + 1;
        break;

    case 2:
        func_8005BD1C(actor);
        heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC) & 0xFF;
        B(actor, 0x3AA) = heading;
        turn = 0x1C0 - (int)heading;
        H(actor, 0x26) = (unsigned short)((turn % 256) << 4);

        S(actor, 2) = S(actor, 2) + S(actor, 0x3CA);
        S(actor, 6) = S(actor, 6) + S(actor, 0x3CE);
        S(actor, 4) = (signed short)((S(actor, 4) * 8) / 10);
        S(actor, 0x3CA) = (signed short)((S(actor, 0x3CA) * 9) / 10);
        S(actor, 0x3CE) = (signed short)((S(actor, 0x3CE) * 9) / 10);

        func_8003DB08(actor);
        if (B(actor, 0x390) == 0xF) { /* frame 15 (0.5 s) */
            S(actor, 4) = 0;
            if (B(actor, 0x39B) != 0 && func_8001E6B8(3) == 0) {
                func_800363B8(actor);
            } else {
                B(actor, 0x397) = B(actor, 0x397) + 1;
            }
        }
        break;

    case 3:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0) {
            func_8005BA30(actor, 0x24, 0x11);
            B(actor, 0x396) = 0x22;
            B(actor, 0x397) = 3;
        }
        break;
    }
}
