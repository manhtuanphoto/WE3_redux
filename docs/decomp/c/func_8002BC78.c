/*
 * func_8002BC78 - Player passing / kick-execution substate machine.
 *
 * Purpose:
 *   Drives a player's pass/kick action across substates 0..18: initializes target
 *   and kick windup (0..2), executes forward/side/backheel pass animations and
 *   ball release (3, 5, 6, 7), and optionally performs a post-pass forward run
 *   follow-through (16..18) before resetting to idle/dribble.
 *
 * Arguments:
 *   actor (unsigned char *): player actor struct (1000 bytes).
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F8000F8/FC (s16, W): predicted target X/Z for follow-through run.
 *   - 0x1F8002A8 (u8, R): last-touch actor ID.
 *   - 0x1F8002CF[2] (u8, R): team possession flags indexed by side.
 *   - 0x1F8002F4 (u8 *, R): pointer to ball actor.
 *   - 0x1F800332 (u8, R): timer/flag copied to actor+0x3B0 in substate 0.
 *
 * Actor fields reads/writes:
 *   - actor: +0x02/+0x06 (s16 x/z, R), +0x38D (u8 id, R), +0x38E (u16 anim id, R),
 *            +0x390 (u8 anim frame, R), +0x392 (u8 team slot, R),
 *            +0x396 (u8 state, W), +0x397 (u8 substate, R/W),
 *            +0x398 (u8 side, R), +0x399 (u8 attack direction flag, R),
 *            +0x3A0 (u32 speed, R/W), +0x3A7 (u8, W=1),
 *            +0x3AA (u8 body heading, R/W), +0x3AB (u8 target heading, R/W),
 *            +0x3AC (u8 mirror/foot, R), +0x3B0 (u8 timer, W),
 *            +0x3C4 (u16 follow-through/kick type flags, R),
 *            +0x3D0 (u8 timer, R/W), +0x3D1 (u8, W=4).
 *   - ball:  +0x3A4 (u8 ball move heading, R).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x1000 (4096 raw = 16.384 m) = forward run target offset in substates 17..18.
 *   - 0x32A1 (12961 raw = 51.844 m) = goal-line X limit ending follow-through run.
 *   - 0x5A (90 frames = 3.00 s) = follow-through timer in substate 17.
 *   - 0x30 (48 raw/frame = 5.76 m/s) = speed set on exit in substate 7.
 *   - 0x28 (40 angle units = 56.25 deg) = turn offset after side pass in substate 5.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_8010C32B[];

extern void func_8002C204(unsigned char *actor);
extern void func_80054A1C(unsigned char *actor);
extern void func_8002C28C(unsigned char *actor);
extern void func_8005BD1C(unsigned char *actor);
extern void func_80047F6C(unsigned char *actor, int a1);
extern int func_8005385C(unsigned char *actor, int a1);
extern int func_80053B44(unsigned char *actor);
extern void func_8002D9F4(unsigned char *actor);
extern int func_8005B7CC(int target, int cur, int step);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern int func_8001E6B8(int n);
extern void func_800479B8(unsigned char *actor, int flag);
extern void func_8005B908(unsigned char *actor);
extern void func_800498AC(unsigned char *actor);

void func_8002BC78(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *ball;
    unsigned int substate;

    substate = B(actor, 0x397);
    B(actor, 0x3A7) = 1;

    if (substate < 0x13U) {
        switch (substate) {
        case 0:
            B(actor, 0x3D1) = 4;
            func_8002C204(actor);
            func_80054A1C(actor);
            B(actor, 0x3B0) = B(scratch, 0x332);
            if (B(actor, 0x38D) == B(scratch, 0x2A8)) {
                func_8002C28C(actor);
            } else {
                B(actor, 0x397) = 1;
            }
            func_8005BD1C(actor);
            break;

        case 1:
        case 2:
            func_8005BD1C(actor);
            func_80047F6C(actor, 1);
            if (func_8005385C(actor, 3) != 0) {
                func_8002C28C(actor);
            }
            break;

        case 3:
        {
            unsigned int frame;

            func_8005BD1C(actor);
            frame = B(actor, 0x390);
            if (frame == 9) {
                if (func_80053B44(actor) != 0) {
                    func_8002D9F4(actor);
                    ball = (unsigned char *)W(scratch, 0x2F4);
                    B(actor, 0x3AB) = B(ball, 0x3A4);
                }
            } else if (frame == 0x16) {
                if ((unsigned short)(H(actor, 0x3C4) - 2) < 2U) {
                    B(actor, 0x397) = 0x10;
                } else {
                    B(actor, 0x396) = 0x18;
                    B(actor, 0x397) = 0;
                }
            }

            if (B(actor, 0x390) < 0xEU) {
                B(actor, 0x3AA) = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 5);
            }
            if (B(actor, 0x390) < 9U && B(actor, 0x38D) != B(scratch, 0x2A8)) {
                break;
            }
            if (W(actor, 0x3A0) < 0xDU) {
                W(actor, 0x3A0) = 0;
            } else {
                W(actor, 0x3A0) -= 0xC;
            }
            break;
        }

        case 5:
        {
            unsigned int frame;

            func_8005BD1C(actor);
            frame = B(actor, 0x390);
            if (frame == 5) {
                if ((unsigned short)(H(actor, 0x3C4) - 2) < 2U) {
                    B(actor, 0x397) = 0x10;
                }
            } else if (frame < 6U) {
                if (frame == 4 && func_80053B44(actor) != 0) {
                    int delta;
                    func_8002D9F4(actor);
                    ball = (unsigned char *)W(scratch, 0x2F4);
                    delta = (B(actor, 0x3AC) != 0) ? -0x28 : 0x28;
                    B(actor, 0x3AB) = (unsigned char)(B(ball, 0x3A4) + delta);
                }
            } else if (frame == 0x10) {
                B(actor, 0x396) = 0x18;
                B(actor, 0x397) = 0;
            }

            if (B(actor, 0x390) >= 5U) {
                if (W(actor, 0x3A0) < 0x11U) {
                    W(actor, 0x3A0) = 0;
                } else {
                    W(actor, 0x3A0) -= 0x10;
                }
            }
            if (B(actor, 0x390) < 9U) {
                B(actor, 0x3AA) = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 5);
            }
            break;
        }

        case 6:
            func_8005BD1C(actor);
            if (B(actor, 0x390) >= 3U) {
                func_8005BA30(actor, 0x13, 2);
                if (func_80053B44(actor) != 0) {
                    func_8002D9F4(actor);
                    ball = (unsigned char *)W(scratch, 0x2F4);
                    B(actor, 0x3AB) = B(ball, 0x3A4);
                }
                B(actor, 0x397) = 7;
            }
            B(actor, 0x3AA) = (unsigned char)func_8005B7CC((B(actor, 0x3AB) + 0x80) & 0xFF, B(actor, 0x3AA), 0xC);
            if (W(actor, 0x3A0) < 0xDU) {
                W(actor, 0x3A0) = 0;
            } else {
                W(actor, 0x3A0) -= 0xC;
            }
            break;

        case 7:
            func_8005BD1C(actor);
            if (B(actor, 0x390) == 0xB) {
                if ((unsigned short)(H(actor, 0x3C4) - 2) < 2U) {
                    B(actor, 0x397) = 0x10;
                } else {
                    func_8005BA30(actor, 0x26, 0);
                    W(actor, 0x3A0) = 0x30;
                    B(actor, 0x396) = 0x18;
                    B(actor, 0x397) = 0;
                }
            }
            if (B(actor, 0x390) < 7U) {
                int step = (B(actor, 0x3AC) != 0) ? 3 : -3;
                B(actor, 0x3AA) = (unsigned char)(B(actor, 0x3AA) + step);
            }
            if (W(actor, 0x3A0) < 0xDU) {
                W(actor, 0x3A0) = 0;
            } else {
                W(actor, 0x3A0) -= 0xC;
            }
            break;

        case 16:
        {
            int offset;
            int stat_val;

            if (H(actor, 0x38E) != 0x26) {
                func_8005BA30(actor, 0x26, 1);
            }
            offset = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
            stat_val = B(D_8010C32B, offset) & 0xF;
            B(actor, 0x3D0) = (unsigned char)(func_8001E6B8((9 - stat_val) << 1) + 3);
            B(actor, 0x397)++;
            break;
        }

        case 17:
            func_8005BD1C(actor);
            S(scratch, 0xF8) = (signed short)(S(actor, 2) + (B(actor, 0x399) != 0 ? -0x1000 : 0x1000));
            S(scratch, 0xFC) = S(actor, 6);
            func_800479B8(actor, 0);
            if (B(actor, 0x3D0) == 0) {
                B(actor, 0x3D0) = 0x5A;
                B(actor, 0x397)++;
            }
            break;

        case 18:
        {
            int x_check;

            func_8005BD1C(actor);
            S(scratch, 0xF8) = (signed short)(S(actor, 2) + (B(actor, 0x399) != 0 ? -0x1000 : 0x1000));
            S(scratch, 0xFC) = S(actor, 6);
            func_800479B8(actor, 1);

            x_check = (B(actor, 0x399) != 0) ? -S(actor, 2) : S(actor, 2);
            if (x_check >= 0x32A1 || B(scratch, 0x2CF + B(actor, 0x398)) != 1) {
                B(actor, 0x396) = 0;
                B(actor, 0x397) = 0;
            } else if (B(actor, 0x3D0) == 0) {
                B(actor, 0x396) = 0x18;
                B(actor, 0x397) = 0;
            }
            break;
        }

        case 4:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        default:
            break;
        }
    }

    func_8005B908(actor);
    func_800498AC(actor);
}
