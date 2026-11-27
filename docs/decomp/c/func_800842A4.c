/*
 * func_800842A4
 * Purpose: Set-piece taker / free-kick or goal-kick state handler (states 0x2C / 0x30).
 *          Manages waiting/facing ball (substate 0), aiming and pad input selection (substate 1),
 *          and kick animation + ball launch at anim frame 26 (substate 2).
 * Arguments:
 *   actor - pointer to 1000-byte player actor struct
 * Return value:
 *   void
 * Scratchpad reads/writes:
 *   R: 0x16..0x19 (pad buttons 1), 0x1A..0x1D (pad buttons 2), 0xF8/0xFC (target x/z),
 *      0x29A (match phase), 0x29B (phase mode), 0x2AC, 0x2AE, 0x2B0 (ball event),
 *      0x2F4 (ball actor ptr)
 *   W: 0xF2 (target y = -512 / -2.048 m), 0x294 (aim timer), 0x2B0 (ball event = 1 or 7),
 *      0x2C0 (= 1), 0x2CF[side] (= 0), 0x2F2 (= 1), 0x309 (= 2)
 * Actor reads/writes:
 *   R: +0x02/+0x06 (x/z), +0x38D (actor id), +0x38E (anim id), +0x390 (anim frame),
 *      +0x397 (substate), +0x398 (side), +0x39A (CPU flag), +0x3A0 (speed),
 *      +0x3AA (body heading), +0x3AB (target heading), +0x3D4 (input flags), +0x3D6 (button mask)
 *   W: +0x396 (state), +0x397 (substate), +0x3A0 (speed), +0x3AA, +0x3AB,
 *      +0x3C4 (= 0), +0x3C6 (= 0), +0x3D6
 *   Ball actor W: +0x3A4 (ball heading), +0x3BC (= 1)
 * Constants:
 *   0x1F (31 frames = 1.03 s) CPU decision delay;
 *   0xB4..0xCB (180..203) kick power, 0x24..0x27 (36..39) kick lift;
 *   -0x200 (-512 raw = -2.048 m height) scratch 0xF2.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCH  ((unsigned char *)0x1F800000)

extern int D_800E7658;

extern int func_8001E6B8(int n);
extern void func_800489A8(unsigned char *actor);
extern void func_80049A94(unsigned char *actor, int a1, int a2, int x, int z);
extern void func_8004B0F0(unsigned char *actor, unsigned int heading, unsigned int power, unsigned int lift);
extern void func_8004B278(void);
extern void func_800512B8(unsigned char *actor, int mode);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);
extern void func_8005B908(unsigned char *actor);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005BD1C(unsigned char *actor);
extern void func_8005C7EC(unsigned char *actor);
extern void func_80076D44(void);
extern void func_80079BA0(unsigned char *actor);
extern void func_800AB620(int sfx);

void func_800842A4(unsigned char *actor) {
    unsigned char *ball;
    unsigned int ang;
    unsigned int spd;
    unsigned short mask;
    int side_off;
    int r_pwr;
    int r_lift;

    switch (B(actor, 0x397)) {
    case 0:
        W(actor, 0x3A0) = 0;
        if (B(actor, 0x38D) == B(SCRATCH, 0x2AC) && W(SCRATCH, 0x2B0) == 4) {
            if (func_8001E6B8(2) != 0) {
                func_8005BA30(actor, 0x38, 0);
                B(actor, 0x396) = 0x2C;
                B(actor, 0x397) = 1;
                break;
            }
        }

        if (B(actor, 0x38D) == B(SCRATCH, 0x2AE)) {
            if (H(actor, 0x38E) != 1) {
                func_8005BA30(actor, 1, func_8001E6B8(0x15) & 0xFF);
            }
            func_8005BD1C(actor);
            if (B(SCRATCH, 0x29A) == 1) {
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 0;
            }
            break;
        }

        if (B(SCRATCH, 0x29B) == 0) {
            if (H(actor, 0x38E) != 1) {
                func_8005BA30(actor, 1, func_8001E6B8(0x15) & 0xFF);
            }
            func_8005BD1C(actor);
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            ang = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6)) & 0xFF;
            B(actor, 0x3AA) = func_8005B7CC(ang, B(actor, 0x3AA), 0x10);
            spd = W(actor, 0x3A0);
            if (spd < 0x13) {
                W(actor, 0x3A0) = 0;
            } else {
                W(actor, 0x3A0) = spd - 0x12;
            }
        } else {
            B(actor, 0x396) = 1;
            B(actor, 0x397) = 0;
        }
        func_8005B908(actor);
        break;

    case 1:
        func_800512B8(actor, 4);
        if (D_800E7658 < 0x10) {
            H(SCRATCH, 0x294) = 0;
            break;
        }

        H(SCRATCH, 0x294) = H(SCRATCH, 0x294) + 1;
        func_8005C7EC(actor);

        if (H(actor, 0x3D4) & 0x8000) {
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            B(ball, 0x3A4) = B(ball, 0x3A4) + 1;
        }
        if (H(actor, 0x3D4) & 0x2000) {
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            B(ball, 0x3A4) = B(ball, 0x3A4) - 1;
        }

        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        B(ball, 0x3A4) = func_8005B7CC(B(ball, 0x3A4), B(actor, 0x3AB), 0x10);

        /* CPU aim/action selection after 31 frames (1.03 s) */
        if (B(actor, 0x39A) != 0 && H(SCRATCH, 0x294) >= 0x1F) {
            side_off = B(actor, 0x398) << 1;
            if (func_8001E6B8(2) != 0) {
                H(actor, 0x3D6) = H(SCRATCH, 0x16 + side_off);
            } else {
                H(actor, 0x3D6) = H(SCRATCH, 0x1A + side_off);
            }
        }

        side_off = B(actor, 0x398) << 1;
        mask = H(actor, 0x3D6);
        if (H(SCRATCH, 0x1A + side_off) & mask) {
            B(SCRATCH, 0x309) = 2;
            B(SCRATCH, 0x2C0) = 1;
            func_8005BA30(actor, 0xF, 0);
            B(actor, 0x397) = B(actor, 0x397) + 1;
        } else if (H(SCRATCH, 0x16 + side_off) & mask) {
            H(actor, 0x3C4) = 0;
            B(SCRATCH, 0x2CF + B(actor, 0x398)) = 0;
            W(SCRATCH, 0x2B0) = 1;
            B(actor, 0x396) = 0x30;
            B(actor, 0x397) = 2;
        }
        break;

    case 2:
        func_8005BD1C(actor);
        func_80079BA0(actor);
        if (B(actor, 0x390) == 0x1A) { /* frame 26: kick ball */
            r_pwr = func_8001E6B8(0x18) + 0xB4; /* 180..203 */
            r_lift = func_8001E6B8(4) + 0x24; /* 36..39 */
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            func_8004B0F0(actor, B(ball, 0x3A4), r_pwr, r_lift);
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            H(ball, 0x3BC) = 1;
            func_800AB620(0x507);
            W(SCRATCH, 0x2B0) = 7;
            func_8004B278();
            S(SCRATCH, 0xF2) = -0x200; /* -2.048 m */
            func_800489A8(actor);
            /* UNCLEAR: $a0 is not reloaded after func_800489A8(actor) before calling func_80049A94 */
            func_80049A94(actor, 1, 1, S(SCRATCH, 0xF8), S(SCRATCH, 0xFC));
            func_80076D44();
            B(SCRATCH, 0x2F2) = 1;
            H(actor, 0x3C6) = 0;
            B(actor, 0x396) = 4;
            B(actor, 0x397) = 4;
            B(actor, 0x3AB) = B(actor, 0x3AA);
        }
        break;
    }

    func_8005B908(actor);
}
