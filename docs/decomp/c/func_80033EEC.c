/*
 * func_80033EEC - Player shot / volley / clearance substate machine.
 *
 * Purpose:
 *   Manages a player's shooting sequence across substates 0..6: initializes shot
 *   type/charge (+0x3CC), waits for ball reach / approach in substates 1..2,
 *   executes strike and optional fake/follow-through branches across substates
 *   3..6 (calling func_80034ACC on kick frames), and updates dribble/touch via
 *   func_8002A83C while in state 9.
 *
 * Arguments:
 *   actor (unsigned char *): player actor struct (1000 bytes).
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F80001A[2] (u16, R): controller button hold mask indexed by side.
 *   - 0x1F800042 (s16, R): predicted ball X threshold (0x3301 = 52.228 m).
 *   - 0x1F8002A8 (u8, R): last-touch actor ID.
 *
 * Actor fields reads/writes:
 *   - actor: +0x02 (s16 x, R), +0x38D (u8 id, R), +0x38E (u16 anim id, R),
 *            +0x390 (u8 anim frame, R), +0x396 (u8 state, R/W),
 *            +0x397 (u8 substate, R/W), +0x398 (u8 side, R),
 *            +0x39B (u8 flag, R), +0x3A0 (u32 speed, R/W),
 *            +0x3A4 (u8 move heading, W), +0x3A7 (u8, W=1),
 *            +0x3AA (u8 body heading, R/W), +0x3AB (u8 target heading, R/W),
 *            +0x3AC (u8 mirror/foot, R), +0x3B0 (u8 timer, R/W),
 *            +0x3B1 (u8, W=0), +0x3C4 (u16 charge counter, R/W),
 *            +0x3C6 (u16 shot/touch flags, R/W), +0x3CC (s16 shot kind, R/W),
 *            +0x3CE (s16 entry speed, R/W), +0x3D1 (u8, W=4),
 *            +0x3D6 (u16 shot button mask, R).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x3301 (13057 raw = 52.228 m) = field-X cutoff in substate 2.
 *   - 0x32A1 (12961 raw = 51.844 m) = goal-line X cutoff in substate 3.
 *   - 180 = speed divisor applied to D_800C6CB0[frame] * entry_speed in substate 4.
 *   - 0x12 (18) = deceleration per frame in substate 6.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800C6CD0[];
extern unsigned char D_800C6CB0[];
extern unsigned char D_800C6C44[];

extern void func_8005BA78(unsigned char *actor, int anim, int frame);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005BD1C(unsigned char *actor);
extern void func_8004840C(unsigned char *actor);
extern void func_800479B8(unsigned char *actor, int flag);
extern int func_8005385C(unsigned char *actor, int a1);
extern void func_80034470(unsigned char *actor);
extern int func_8005B7CC(int target, int cur, int step);
extern int func_80053B44(unsigned char *actor);
extern void func_80034ACC(unsigned char *actor);
extern int func_8001E6B8(int n);
extern void func_8002A83C(unsigned char *actor, int flags);
extern void func_8005B908(unsigned char *actor);

void func_80033EEC(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned int substate = B(actor, 0x397);

    B(actor, 0x3A7) = 1;

    if (substate < 7U) {
        switch (substate) {
        case 0:
        {
            unsigned int anim = H(actor, 0x38E);

            B(actor, 0x3D1) = 4;
            H(actor, 0x3C4) = 0;
            H(actor, 0x3C6) = 0;
            B(actor, 0x3B1) = 0;
            B(actor, 0x3B0) = 0;

            if (anim == 4) {
                H(actor, 0x3CC) = 2;
            } else if ((int)anim < 5) {
                if (anim == 1) {
                    H(actor, 0x3CC) = 0;
                } else {
                    H(actor, 0x3CC) = 1;
                }
            } else if (anim == 0x58) {
                H(actor, 0x3CC) = 0;
            } else {
                H(actor, 0x3CC) = 1;
            }

            if (B(actor, 0x38D) == B(scratch, 0x2A8)) {
                if (W(actor, 0x3A0) != 0) {
                    func_8005BA78(actor, 0x26, B(actor, 0x390));
                } else if (H(actor, 0x38E) != 1) {
                    func_8005BA30(actor, 1, 0);
                }
                B(actor, 0x397) = 2;
            } else {
                B(actor, 0x397) = 1;
            }

            func_8005BD1C(actor);
            B(actor, 0x3A4) = B(actor, 0x3AA);
            break;
        }

        case 1:
            func_8005BD1C(actor);
            if ((H(scratch, 0x1A + (B(actor, 0x398) << 1)) & H(actor, 0x3D6)) != 0) {
                H(actor, 0x3C4)++;
            }
            func_8004840C(actor);
            func_800479B8(actor, 1);
            B(actor, 0x3B0)++;
            if (func_8005385C(actor, 4) != 0) {
                B(actor, 0x397) = 2;
            }
            break;

        case 2:
            func_8005BD1C(actor);
            if ((H(scratch, 0x1A + (B(actor, 0x398) << 1)) & H(actor, 0x3D6)) != 0) {
                H(actor, 0x3C4)++;
            }
            B(actor, 0x3B0)++;
            if (B(actor, 0x3B0) < 6U) {
                int x42 = S(scratch, 0x42);
                if (x42 < 0) {
                    x42 = -x42;
                }
                if (x42 < 0x3301) {
                    break;
                }
            }
            func_80034470(actor);
            break;

        case 3:
            func_8005BD1C(actor);
            B(actor, 0x3AA) = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0x10);
            if ((H(scratch, 0x1A + (B(actor, 0x398) << 1)) & H(actor, 0x3D6)) != 0) {
                H(actor, 0x3C4)++;
            }
            if (B(actor, 0x390) == 3) {
                if (func_80053B44(actor) != 0) {
                    func_80034ACC(actor);
                }
                B(actor, 0x3AB) = B(actor, 0x3AA);
                if (B(actor, 0x39B) == 0) {
                    int ax = S(actor, 2);
                    if (ax < 0) {
                        ax = -ax;
                    }
                    if (ax < 0x32A1) {
                        goto advance_substate3;
                    }
                }
                if (S(actor, 0x3CC) == 2 && func_8001E6B8(2) == 0) {
                    B(actor, 0x396) = 0x14;
                    B(actor, 0x397) = 0x10;
                    break;
                }
advance_substate3:
                S(actor, 0x3CE) = (signed short)W(actor, 0x3A0);
                B(actor, 0x397)++;
            }
            break;

        case 4:
        {
            unsigned int frame;
            int delta;
            int heading;

            func_8005BD1C(actor);
            if (B(actor, 0x390) == 0x1C) {
                B(actor, 0x396) = 0x18;
                B(actor, 0x397) = 0;
            }
            frame = B(actor, 0x390);
            delta = D_800C6CD0[frame];
            if (B(actor, 0x3AC) != 0) {
                heading = B(actor, 0x3AB) - delta;
            } else {
                heading = B(actor, 0x3AB) + delta;
            }
            B(actor, 0x3AA) = (unsigned char)heading;
            W(actor, 0x3A0) = (unsigned int)((D_800C6CB0[frame] * S(actor, 0x3CE)) / 180);
            break;
        }

        case 5:
            func_8005BD1C(actor);
            if (B(actor, 0x390) < 0x1BU) {
                if ((H(scratch, 0x1A + (B(actor, 0x398) << 1)) & H(actor, 0x3D6)) != 0) {
                    H(actor, 0x3C4)++;
                }
                B(actor, 0x3AA) = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xA);
                if (B(actor, 0x390) == 0x1A && func_80053B44(actor) != 0) {
                    func_80034ACC(actor);
                }
            }
            if (B(actor, 0x390) == 0x1B) {
                if (B(actor, 0x39B) != 0 && S(actor, 0x3CC) == 2 && func_8001E6B8(2) == 0) {
                    B(actor, 0x396) = 0x14;
                    B(actor, 0x397) = 0x10;
                } else {
                    H(actor, 0x3C6) = 0;
                    B(actor, 0x396) = 4;
                    B(actor, 0x397) = 4;
                    B(actor, 0x3AB) = B(actor, 0x3AA);
                }
            }
            break;

        case 6:
        {
            unsigned int frame;

            func_8005BD1C(actor);
            frame = B(actor, 0x390);
            if (frame < 8U) {
                int delta = D_800C6C44[frame];
                B(actor, 0x3AA) = (unsigned char)(B(actor, 0x3AC) != 0 ? (B(actor, 0x3AB) - delta) : (B(actor, 0x3AB) + delta));
                if (B(actor, 0x390) == 7 && func_80053B44(actor) != 0) {
                    func_80034ACC(actor);
                }
                break;
            }
            if (frame == 9) {
                func_8005BA30(actor, 0x54, 0x10);
            }
            if (B(actor, 0x390) == 0x1A) {
                B(actor, 0x396) = 0x18;
                B(actor, 0x397) = 0;
            }
            if (W(actor, 0x3A0) < 0x13U) {
                W(actor, 0x3A0) = 0;
            } else {
                W(actor, 0x3A0) -= 0x12;
            }
            break;
        }
        }
    }

    if (B(actor, 0x396) == 9) {
        func_8002A83C(actor, B(actor, 0x3C6));
    }
    func_8005B908(actor);
}
