/*
 * func_80093644 - Goalkeeper high/diving save execution state handler (substates 0..1).
 *
 * Purpose:
 *   Executes a goalkeeper diving save:
 *   - Substate 0: Waits until reaction timer (0x31C >= 17 - reaction/10, else calls
 *                 func_8006EC5C), then computes horizontal/vertical dive velocities toward
 *                 (+0x3C2, +0x3C4, +0x3C6) over 10/5 frames, registers gravity via
 *                 func_80094474, starts anim 0x77, and advances to substate 1.
 *   - Substate 1: Advances anim 0x77 across frames 1..22: turns body toward target heading
 *                 (+0x3AB) during frames 1..10, integrates XZ/Y velocity, performs ball
 *                 parry/deflection at frame 9 (func_80094EEC radius 0x50 = 0.32 m,
 *                 kicking via func_8004B0F0 if not dead-ball actor), reverses Y motion on
 *                 frames 11..21, decelerates on frame 22+ (70% then 92%/frame), and on
 *                 anim loop (frame 0) rotates 90 deg and transitions to recovery state 0x8E.
 *   - Post-switch: Calls func_800954E8 and func_8009462C using scaled ball vertical speed.
 *
 * Arguments:
 *   actor (unsigned char*): goalkeeper actor struct (1000 bytes).
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F8000F0 / 0x1F8000F2 / 0x1F8000F4 (s16, R): scratchpad origin X/Y/Z.
 *   - 0x1F8002AC (u8, R): dead-ball / set-piece actor ID.
 *   - 0x1F8002F4 (u8*, R): pointer to ball actor.
 *   - 0x1F80031C (s16, R): ball timer (frames since last touch).
 *
 * Actor / global fields reads/writes:
 *   - actor: +0x02/+0x04/+0x06 (s16 x/y/z, R/W), +0x08/+0x0C/+0x10 (s32 vx/vy/vz, R/W),
 *            +0x26 (s16 rotation, W), +0x38D (u8 id, R), +0x390 (u8 anim frame, R),
 *            +0x392 (u8 team slot, R), +0x396 (u8 state, W=0x8E),
 *            +0x397 (u8 substate, R/W), +0x398 (u8 side, R),
 *            +0x3AA (u8 body heading, R/W), +0x3AB (u8 target heading, R),
 *            +0x3AC (u8 mirror/foot, R), +0x3B4 (u8 max speed, R), +0x3B9 (u8 reaction stat, R),
 *            +0x3C2/+0x3C4/+0x3C6 (s16 target x/y/z, R).
 *   - ball:  +0x0C (s32 vy, R), +0x3A0 (u32 speed, R), +0x3A4 (u8 move heading, R).
 *   - D_8010C32F[] (u8, R): GK stat byte (low nibble) indexed by slot*12 + side*264.
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x50 (80) = 0.32 m ball parry touch radius in frame 9.
 *   - 0x38 (56) = minimum parry kick power (<<8 in func_8004B0F0).
 *   - 736 (0x2E0) = ball vertical velocity scale factor per lift unit.
 *   - 0x40 (64 = 90 deg) = body heading turn on landing/recovery (anim 0x63, state 0x8E).
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_8010C32F[];

extern void func_8006EC5C(unsigned char *actor);
extern void func_8005C2AC(unsigned char *actor, int anim, int a2, int heading, int foot);
extern int func_800B1128(int val);
extern int func_8001E694(int angle, int scale);
extern int func_8001E5A4(int angle, int scale);
extern void func_80094474(unsigned char *vy_ptr, int a1, int gravity);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005BD1C(unsigned char *actor);
extern int func_8005B7CC(int target, int cur, int step);
extern int func_80094EEC(unsigned char *actor, int radius);
extern int func_8001E6B8(int n);
extern void func_8004B0F0(unsigned char *actor, int heading, int power, int lift);
extern void func_80095090(unsigned char *actor);
extern void func_80057A94(unsigned char *actor);
extern void func_800954E8(unsigned char *actor);
extern void func_8009462C(unsigned char *actor, int a1, int heading, int a3, int a4);

void func_80093644(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *ball;
    int substate;
    int dx, dz;
    unsigned int spd, max_spd;
    int row, stat;
    int ang;
    int heading, turn;
    unsigned int frame;
    int ball_lift;

    substate = B(actor, 0x397);
    if (substate == 0) {
        if (S(scratch, 0x31C) < (17 - (int)(B(actor, 0x3B9) / 10U))) {
            func_8006EC5C(actor);
            return;
        }
        func_8005C2AC(actor, 0x77, 9, B(actor, 0x3AB), B(actor, 0x3AC));
        dx = S(actor, 0x3C2) - S(scratch, 0xF0);
        dz = S(actor, 0x3C6) - S(scratch, 0xF4);
        spd = (unsigned int)func_800B1128(dx * dx + dz * dz) / 10U;

        row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        stat = B(D_8010C32F, row) & 0xF;
        max_spd = (unsigned int)(B(actor, 0x3B4) + (stat >> 1) - 8);
        if (max_spd < spd) {
            spd = max_spd;
        }

        ang = (B(actor, 0x3AB) - 0x80) & 0xFF;
        SW(actor, 8) = func_8001E694(ang, (int)spd);
        SW(actor, 0xC) = (S(actor, 0x3C4) - S(scratch, 0xF2)) / 5;
        func_80094474(actor + 0xC, 0xFF, -0x10 - (stat >> 1));
        SW(actor, 0x10) = func_8001E5A4(ang, (int)spd);
        func_8005BA30(actor, 0x77, 0);
        S(actor, 4) = 0;
        S(actor, 2) = (short)(S(actor, 2) + SW(actor, 8));
        S(actor, 6) = (short)(S(actor, 6) + SW(actor, 0x10));
        B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
    } else if (substate == 1) {
        func_8005BD1C(actor);
        frame = B(actor, 0x390);
        switch (frame) {
        case 1:
        case 2:
        case 3:
        case 4:
            S(actor, 2) = (short)(S(actor, 2) + SW(actor, 8));
            S(actor, 6) = (short)(S(actor, 6) + SW(actor, 0x10));
            heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
            B(actor, 0x3AA) = (unsigned char)heading;
            turn = 0x1C0 - (heading & 0xFF);
            S(actor, 0x26) = (short)((turn % 256) * 16);
            break;

        case 5:
        case 6:
        case 7:
        case 8:
        case 10:
            heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
            B(actor, 0x3AA) = (unsigned char)heading;
            turn = 0x1C0 - (heading & 0xFF);
            S(actor, 0x26) = (short)((turn % 256) * 16);
            S(actor, 2) = (short)(S(actor, 2) + SW(actor, 8));
            S(actor, 4) = (short)(S(actor, 4) + SW(actor, 0xC));
            S(actor, 6) = (short)(S(actor, 6) + SW(actor, 0x10));
            break;

        case 9:
            S(actor, 2) = (short)(S(actor, 2) + SW(actor, 8));
            S(actor, 4) = (short)(S(actor, 4) + SW(actor, 0xC));
            S(actor, 6) = (short)(S(actor, 6) + SW(actor, 0x10));
            if (func_80094EEC(actor, 0x50) != 0 && B(actor, 0x38D) != B(scratch, 0x2AC)) {
                unsigned int denom = (unsigned int)(func_8001E6B8(0x80) + 0x200);
                unsigned int pwr;
                ball = (unsigned char *)W(scratch, 0x2F4);
                pwr = W(ball, 0x3A0) / denom;
                if (pwr < 0x39U) {
                    pwr = 0x38U;
                }
                func_8004B0F0(actor, B(ball, 0x3A4), (int)pwr, (int)((pwr >> 4) + 0x10));
                func_80095090(actor);
                func_80057A94(actor);
            }
            SW(actor, 0xC) = SW(actor, 0xC) / 2;
            break;

        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
            S(actor, 2) = (short)(S(actor, 2) + SW(actor, 8));
            S(actor, 4) = (short)(S(actor, 4) - SW(actor, 0xC));
            S(actor, 6) = (short)(S(actor, 6) + SW(actor, 0x10));
            break;

        case 22:
            SW(actor, 8) = (SW(actor, 8) * 7) / 10;
            SW(actor, 0x10) = (SW(actor, 0x10) * 7) / 10;
            /* fall through */
        default:
            S(actor, 4) = 0;
            S(actor, 2) = (short)(S(actor, 2) + SW(actor, 8));
            SW(actor, 8) = (SW(actor, 8) * 92) / 100;
            S(actor, 6) = (short)(S(actor, 6) + SW(actor, 0x10));
            SW(actor, 0x10) = (SW(actor, 0x10) * 92) / 100;
            if (B(actor, 0x390) == 0) {
                if (B(actor, 0x3AC) != 0) {
                    heading = B(actor, 0x3AA) + 0x40;
                } else {
                    heading = B(actor, 0x3AA) - 0x40;
                }
                B(actor, 0x3AA) = (unsigned char)heading;
                turn = 0x1C0 - (heading & 0xFF);
                S(actor, 0x26) = (short)((turn % 256) * 16);
                func_8005BA30(actor, 0x63, 0x12);
                B(actor, 0x396) = 0x8E;
                B(actor, 0x397) = 1;
            }
            break;
        }
    }

    func_800954E8(actor);
    ball = (unsigned char *)W(scratch, 0x2F4);
    ball_lift = SW(ball, 0xC) / 736;
    if (ball_lift < 0) {
        ball_lift = -ball_lift;
    }
    func_8009462C(actor, 0xA, B(ball, 0x3A4), 0, (ball_lift * 7) / 10);
}
