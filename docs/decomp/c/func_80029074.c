/*
 * func_80029074
 * Purpose: Updates a player actor's on-ball / pass / kick / turn / recovery action
 *          state machine across 10 substates (+0x397 = 0..9).
 * Arguments:
 *   - actor (unsigned char *): pointer to 1000-byte player actor struct
 * Return value: void
 * Actor fields read/written:
 *   - +0x02 (s16): X position (250 raw = 1 m)
 *   - +0x06 (s16): Z position (250 raw = 1 m)
 *   - +0x38D (u8): actor ID
 *   - +0x38E (u16): current animation ID
 *   - +0x390 (u8): current animation frame
 *   - +0x396 (u8): actor state (set to 1, 0x14, or 0x18 on transitions)
 *   - +0x397 (u8): substate (0..9, updated on transitions)
 *   - +0x398 (u8): team side (0/1)
 *   - +0x399 (u8): attack direction flag (0/1 -> target goal angle 0 or 128)
 *   - +0x39A (u8): CPU/AI or action flag
 *   - +0x39B (u8): secondary flag (triggers transition to state 0x14 / substate 0x10)
 *   - +0x3A0 (u32): movement speed (decelerated by 16 or 18 per frame, or read from D_800C6C04)
 *   - +0x3A4 (u8): move heading
 *   - +0x3A7 (u8): set to 1
 *   - +0x3AA (u8): body heading
 *   - +0x3AB (u8): target heading
 *   - +0x3AC (u8): mirror/foot flag
 *   - +0x3AE (u16): timer decremented by 2 each call
 *   - +0x3B0 (u8): frame counter in substates 1 and 2
 *   - +0x3B1 (u8): cleared to 0
 *   - +0x3C2 (s16): contact/target flag (cleared when >= 2 or +0x39A == 0)
 *   - +0x3C6 (u16/s16): charge/timer value (initialized to 4, incremented by 2 when bitmask matches)
 *   - +0x3C8 (s16/u16): kick power/target value
 *   - +0x3D0 (u8): substate 7 hold flag
 *   - +0x3D1 (u8): set to 2 in substates 0 and 2
 *   - +0x3D4 (u16): input mask tested against scratchpad 0x12[side]
 * Scratchpad reads/writes:
 *   - 0x1F800012[side] (u16): per-side held button bitmask
 *   - 0x1F80003E (s16): target/ball predicted X
 *   - 0x1F80006E (s16): target/ball predicted Z
 *   - 0x1F8002A8 (u8): last-touch actor ID
 *   - 0x1F8002A9 (u8): current ball owner ID
 *   - 0x1F8002F4 (ptr): pointer to ball actor (+0x3C8 cleared to 0 on kick)
 * Magic numbers:
 *   - 0x21 (33/256 = 46.4 deg): goal-facing angle tolerance in substate 1
 *   - 0x3D (61 frames = 2.03 s): max wait in substate 1 before reverting to state 1
 *   - 0x21 (33 frames = 1.10 s): max wait in substate 2 before transitioning to state 0x18
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCHPAD ((unsigned char *)0x1F800000)

extern unsigned char D_800C6C04[];
extern unsigned char D_800C6C44[];

extern void func_8005BA78(unsigned char *, unsigned int, unsigned int);
extern void func_8005BA30(unsigned char *, unsigned int, unsigned int);
extern void func_80054A1C(unsigned char *);
extern void func_8005BD1C(unsigned char *);
extern void func_8002A7D8(unsigned char *);
extern void func_8004840C(unsigned char *);
extern unsigned int func_8005B36C(int, int, int, int);
extern void func_800479B8(unsigned char *, int);
extern void func_80047F6C(unsigned char *, int);
extern int func_8005385C(unsigned char *, int);
extern void func_8003F7A4(unsigned char *);
extern void func_80029928(unsigned char *);
extern unsigned int func_8005B7CC(unsigned int, unsigned int, unsigned int);
extern int func_80053B44(unsigned char *);
extern int func_8001E6B8(int);
extern unsigned int func_8002AFE0(unsigned int, unsigned int);
extern void func_80029F30(unsigned char *, unsigned int, unsigned int);
extern void func_8002B38C(unsigned char *);
extern void func_8002A83C(unsigned char *, unsigned int);
extern void func_8005B908(unsigned char *);
extern void func_800498AC(unsigned char *);

void func_80029074(unsigned char *actor)
{
    unsigned char *scratch = SCRATCHPAD;
    unsigned char substate;

    B(actor, 0x3A7) = 1;
    substate = B(actor, 0x397);
    H(actor, 0x3AE) = H(actor, 0x3AE) - 2;

    if (substate < 10) {
        switch (substate) {
        case 0: {
            unsigned int nextSub;

            B(actor, 0x3D1) = 2;
            if (B(actor, 0x39A) != 0 && S(actor, 0x3C2) < 2) {
                H(actor, 0x3C6) = 4;
            } else {
                H(actor, 0x3C2) = 0;
                H(actor, 0x3C6) = 4;
            }
            B(actor, 0x3B1) = 0;
            B(actor, 0x3B0) = 0;

            if (B(actor, 0x38D) == B(scratch, 0x2A8)) {
                if (W(actor, 0x3A0) != 0) {
                    func_8005BA78(actor, 0x26, B(actor, 0x390));
                    nextSub = 2;
                } else {
                    if (H(actor, 0x38E) != 1) {
                        func_8005BA30(actor, 1, 0);
                    }
                    nextSub = 2;
                }
            } else {
                func_80054A1C(actor);
                nextSub = 1;
            }

            B(actor, 0x397) = nextSub;
            func_8005BD1C(actor);
            B(actor, 0x3A4) = B(actor, 0x3AA);
            break;
        }

        case 1: {
            unsigned int ang;
            unsigned int goalAng;
            int inCone;

            func_8005BD1C(actor);
            if ((H(scratch, 0x12 + B(actor, 0x398) * 2) & H(actor, 0x3D4)) != 0) {
                H(actor, 0x3C6) = H(actor, 0x3C6) + 2;
            }
            func_8002A7D8(actor);
            func_8004840C(actor);

            ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(scratch, 0x3E), S(scratch, 0x6E));
            goalAng = (unsigned int)B(actor, 0x399) << 7;
            if (((ang - goalAng) & 0xFF) < 0x81U) {
                ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(scratch, 0x3E), S(scratch, 0x6E));
                goalAng = (unsigned int)B(actor, 0x399) << 7;
                inCone = (signed int)((ang - goalAng) & 0xFF) < 0x21;
            } else {
                ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(scratch, 0x3E), S(scratch, 0x6E));
                goalAng = (unsigned int)B(actor, 0x399) << 7;
                inCone = (signed int)((goalAng - ang) & 0xFF) < 0x21;
            }

            if (inCone) {
                func_800479B8(actor, 1);
            } else {
                func_80047F6C(actor, 1);
            }

            B(actor, 0x3B0) = B(actor, 0x3B0) + 1;
            if (B(actor, 0x3B0) >= 0x3D && B(actor, 0x38D) != B(scratch, 0x2A9)) {
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 0;
                break;
            }

            if (func_8005385C(actor, 4) != 0) {
                if ((H(scratch, 0x12 + B(actor, 0x398) * 2) & H(actor, 0x3D4)) != 0 && S(actor, 0x3C6) < 8) {
                    break;
                }
                func_80029928(actor);
            } else {
                func_8003F7A4(actor);
            }
            break;
        }

        case 2:
            B(actor, 0x3D1) = 2;
            func_8005BD1C(actor);
            if (W(actor, 0x3A0) != 0) {
                func_8004840C(actor);
                func_800479B8(actor, 1);
            }
            B(actor, 0x3B0) = B(actor, 0x3B0) + 1;
            if (B(actor, 0x3B0) >= 0x21 && B(actor, 0x38D) != B(scratch, 0x2A9)) {
                B(actor, 0x396) = 0x18;
                B(actor, 0x397) = 0;
                break;
            }
            if ((H(scratch, 0x12 + B(actor, 0x398) * 2) & H(actor, 0x3D4)) != 0) {
                H(actor, 0x3C6) = H(actor, 0x3C6) + 2;
                if (S(actor, 0x3C6) < 8) {
                    break;
                }
            }
            func_80029928(actor);
            break;

        case 3:
            func_8005BD1C(actor);
            if ((H(scratch, 0x12 + B(actor, 0x398) * 2) & H(actor, 0x3D4)) != 0) {
                H(actor, 0x3C6) = H(actor, 0x3C6) + 2;
            }
            func_8002A7D8(actor);
            B(actor, 0x3AA) = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 10);

            if (B(actor, 0x390) == 0x1A && func_80053B44(actor) != 0) {
                unsigned int kickPower;
                unsigned int passArg;
                unsigned int turned;

                if (S(actor, 0x3C8) < 0x101) {
                    passArg = 1;
                    kickPower = 4;
                } else {
                    passArg = (unsigned int)func_8001E6B8(4);
                    kickPower = passArg + 5;
                }
                /* UNCLEAR: in the binary, the delay slot of `jal func_8002AFE0` executes
                 * `andi $a0, $v0, 0xFF` (overwriting actor in $a0 with passArg & 0xFF),
                 * and the subsequent `func_8005B7CC` reuses $a0 without loading the return
                 * value of func_8002AFE0. */
                passArg = func_8002AFE0(passArg & 0xFF, kickPower);
                turned = func_8005B7CC(passArg & 0xFF, B(actor, 0x3AA), 0x30);
                func_80029F30(actor, H(actor, 0x3C8), turned & 0xFF);
            }

            if (B(actor, 0x390) == 0x1A) {
                if (B(actor, 0x39B) != 0 && func_8001E6B8(2) != 0) {
                    B(actor, 0x396) = 0x14;
                    B(actor, 0x397) = 0x10;
                    break;
                }
                B(actor, 0x397) = 4;
                B(actor, 0x3AB) = B(actor, 0x3AA);
            }
            break;

        case 4: {
            unsigned char frame;

            func_8005BD1C(actor);
            frame = B(actor, 0x390);
            W(actor, 0x3A0) = B(D_800C6C04, frame);
            if (frame == 0x2B) {
                func_8005BA30(actor, 1, 0);
                B(actor, 0x396) = 0x18;
                B(actor, 0x397) = 0;
            }
            break;
        }

        case 5: {
            unsigned char frame;

            func_8005BD1C(actor);
            func_8002A7D8(actor);
            if ((H(scratch, 0x12 + B(actor, 0x398) * 2) & H(actor, 0x3D4)) != 0) {
                H(actor, 0x3C6) = H(actor, 0x3C6) + 2;
            }

            frame = B(actor, 0x390);
            if (frame == 9) {
                if (func_80053B44(actor) != 0) {
                    unsigned int power = (unsigned short)(func_8001E6B8(0x10) + 0xB8);
                    unsigned int dir = func_8002AFE0((unsigned int)actor, 3);
                    func_80029F30(actor, power, dir & 0xFF);
                }
                if (B(actor, 0x39B) != 0 && func_8001E6B8(2) != 0) {
                    B(actor, 0x396) = 0x14;
                    B(actor, 0x397) = 0x10;
                }
                frame = B(actor, 0x390);
            }

            if (frame == 0x16) {
                B(actor, 0x396) = 0x18;
                B(actor, 0x397) = 0;
            }

            B(actor, 0x3AA) = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 10);
            if (B(actor, 0x390) >= 0xA) {
                unsigned int spd = W(actor, 0x3A0);
                W(actor, 0x3A0) = (spd < 0x13) ? 0 : (spd - 0x12);
            }
            break;
        }

        case 6:
            func_8005BD1C(actor);
            if ((H(scratch, 0x12 + B(actor, 0x398) * 2) & H(actor, 0x3D4)) != 0) {
                H(actor, 0x3C6) = H(actor, 0x3C6) + 2;
            }
            B(actor, 0x3AA) = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 10);

            if (B(actor, 0x390) == 4 && func_80053B44(actor) != 0) {
                unsigned int power;
                unsigned int dir;
                unsigned char *ball;

                B(actor, 0x3B1) = 0;
                power = (unsigned short)(func_8001E6B8(0x10) + 0xB8);
                dir = func_8002AFE0((unsigned int)actor, 5);
                func_80029F30(actor, power, dir & 0xFF);
                ball = (unsigned char *)W(scratch, 0x2F4);
                H(ball, 0x3C8) = 0;
            }

            if (B(actor, 0x390) == 0x10) {
                B(actor, 0x396) = 0x18;
                B(actor, 0x397) = 0;
            }

            if (B(actor, 0x390) >= 5) {
                unsigned int spd = W(actor, 0x3A0);
                W(actor, 0x3A0) = (spd < 0x11) ? 0 : (spd - 0x10);
            }

            if (B(actor, 0x390) >= 6) {
                B(actor, 0x3AA) = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 2);
            }
            break;

        case 7: {
            unsigned int spd;

            func_8005BD1C(actor);
            spd = W(actor, 0x3A0);
            W(actor, 0x3A0) = (spd < 0x13) ? 0 : (spd - 0x12);
            if (B(actor, 0x3D0) == 0) {
                func_80029928(actor);
            }
            break;
        }

        case 8: {
            unsigned char delta;
            unsigned char target;

            func_8005BD1C(actor);
            if ((H(scratch, 0x12 + B(actor, 0x398) * 2) & H(actor, 0x3D4)) != 0) {
                H(actor, 0x3C6) = H(actor, 0x3C6) + 2;
            }

            delta = B(D_800C6C44, B(actor, 0x390));
            target = B(actor, 0x3AB);
            B(actor, 0x3AA) = (B(actor, 0x3AC) != 0) ? (target - delta) : (target + delta);

            if (B(actor, 0x390) == 7 && func_80053B44(actor) != 0) {
                unsigned int power;
                unsigned int dir;
                unsigned char *ball;

                B(actor, 0x3B1) = 0;
                power = (unsigned short)(func_8001E6B8(0x10) + 0xA0);
                dir = func_8002AFE0((unsigned int)actor, 8);
                func_80029F30(actor, power, dir & 0xFF);
                ball = (unsigned char *)W(scratch, 0x2F4);
                H(ball, 0x3C8) = 0;
            }

            if (B(actor, 0x390) == 0x1A) {
                B(actor, 0x396) = 0x18;
                B(actor, 0x397) = 0;
            }

            if (B(actor, 0x390) >= 8) {
                unsigned int spd = W(actor, 0x3A0);
                W(actor, 0x3A0) = (spd < 0x13) ? 0 : (spd - 0x12);
            }
            break;
        }

        case 9:
            break;
        }
    }

    func_8002B38C(actor);
    func_8002A83C(actor, B(actor, 0x3C6));
    func_8005B908(actor);
    func_800498AC(actor);
}
