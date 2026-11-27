/*
 * func_8007AD20 - Goalkeeper set-piece / goal-kick / dead-ball state machine.
 *
 * Purpose:
 *   Drives a player/goalkeeper actor through substates 0..8 for set-piece / goal-kick
 *   setup, aiming (human input or AI target choice), kicking/punting the ball, and
 *   post-kick recovery before returning to normal play (state 0).
 *
 * Arguments:
 *   actor (unsigned char*): player/GK actor struct (1000 bytes).
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F800016[2] (u16, R): per-side button mask A (pass/low kick).
 *   - 0x1F80001A[2] (u16, R): per-side button mask B (long kick/lofted kick).
 *   - 0x1F800294 (u16, R/W): set-piece wait timer (frames).
 *   - 0x1F80029A (u8, R): match phase (1 = open play).
 *   - 0x1F80029B (u8, R): phase mode / ready flag.
 *   - 0x1F8002A8 (u8, R): last-touch actor ID.
 *   - 0x1F8002AC (u8, R): active set-piece / dead-ball actor ID.
 *   - 0x1F8002AD (u8, R): designated actor ID for substate 0 check.
 *   - 0x1F8002B0 (u32, W): ball event code (6 in substate 6).
 *   - 0x1F8002B4 / 0x1F8002B6 (s16, R): kick landing target x/z passed to func_80049A94.
 *   - 0x1F8002C0 (u8, W): set-piece kick triggered flag (1).
 *   - 0x1F8002F2 (u8, W): UI/radar flag (3 when human).
 *   - 0x1F8002F4 (u8*, R): pointer to ball actor.
 *   - 0x1F800309 (u8, W): set to 2 when human starts kick.
 *   - 0x1F80031C (s16, W): ball timer (set to 4 frames = 0.13 s).
 *   - 0x1F8001F0..0x1F80020F -> 0x1F800210..0x1F80022F (32 bytes, R/W): copied block.
 *
 * Actor fields reads/writes:
 *   - actor: +0x02/+0x06 (s16 x/z, R), +0x26 (s16 rotation, W),
 *            +0x38D (u8 id, R), +0x38E (u16 anim id, R), +0x390 (u8 anim frame, R),
 *            +0x396 (u8 state, W), +0x397 (u8 substate, R/W),
 *            +0x398 (u8 side, R), +0x399 (u8 attack dir, R), +0x39A (u8 AI flag, R),
 *            +0x39B (u8, R), +0x3A0 (u32 speed, R/W), +0x3AA (u8 body heading, R/W),
 *            +0x3AB (u8 target heading, R), +0x3C2 (s16 kick type 0/1, R/W),
 *            +0x3D1 (u8, W=0), +0x3D4 (u16 input mask, R), +0x3D6 (u16 stored mask, R/W).
 *   - ball:  +0x02/+0x06 (s16 x/z, R).
 *
 * Globals:
 *   - D_800E7658 (s32, R): readiness threshold (< 16 waits and resets 0x294).
 *   - D_800CA58C[] (u8, R): per-frame movement speed lookup table during kick anim.
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x2181 (8577) = 34.31 m |z| touchline/width bound in substate 3.
 *   - 0x4000 (16384 = 128^2) = 0.512 m squared XZ distance to ball in substate 3.
 *   - 0xCC8 (3272) = 13.09 m, 0x23E2 (9186) = 36.74 m attack-dir ball X range in substate 5.
 *   - 0x1F (31 frames = 1.03 s) AI aim wait in substate 5.
 *   - 0x80 (128 -> <<8 = 32768) base kick power in substate 6; speed 0x30 (48) on exit.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern int D_800E7658;
extern unsigned char D_800CA58C[];

extern void func_8005BD1C(unsigned char *actor);
extern int func_8001E6B8(int n);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005BA78(unsigned char *actor, int anim, int frame);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8005B7CC(int target, int cur, int step);
extern void func_8005B864(unsigned char *actor, int speed);
extern void func_8005C7EC(void);
extern void func_80050510(int a0);
extern void func_80051A84(unsigned char *dst);
extern void func_8004B0F0(unsigned char *actor, int heading, int power, int lift);
extern void func_8004B278(void);
extern void func_80049A94(unsigned char *actor, int a1, int a2, int x, int z);
extern void func_80076D44(void);

void func_8007AD20(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *ball;
    unsigned int substate;
    int anim;
    int heading;
    int turn;
    int dx, dz;
    int abs_z;
    int ball_x_atk;
    int side_off;
    unsigned short mask;
    int power, lift;
    int i;

    substate = B(actor, 0x397);
    if (substate >= 9) {
        return;
    }

    switch (substate) {
    case 0:
        func_8005BD1C(actor);
        if (B(actor, 0x38D) == B(scratch, 0x2AD)) {
            B(actor, 0x397) = 3;
        } else {
            B(actor, 0x397) = 4;
        }
        W(actor, 0x3A0) = 0;
        break;

    case 1:
        anim = H(actor, 0x38E);
        if ((anim == 0x38 && B(actor, 0x390) < 0xD) ||
            (anim == 0x3C && B(actor, 0x390) < 0x2D)) {
            func_8005BD1C(actor);
        }
        if (B(scratch, 0x29B) != 0) {
            B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
        }
        W(actor, 0x3A0) = 0;
        break;

    case 2:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0) {
            if (H(actor, 0x38E) != 1) {
                func_8005BA30(actor, 1, func_8001E6B8(0x15) & 0xFF);
            }
            B(actor, 0x397) = 4;
        }
        break;

    case 3:
        func_8005BD1C(actor);
        ball = (unsigned char *)W(scratch, 0x2F4);
        heading = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6)) & 0xFF;
        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(heading, B(actor, 0x3AA), 0x10);
        func_8005B864(actor, 0x1C);
        if (B(scratch, 0x2A8) == 0 && B(actor, 0x39B) == 0) {
            abs_z = S(actor, 6);
            if (abs_z < 0) {
                abs_z = -abs_z;
            }
            if (abs_z < 0x2181) {
                ball = (unsigned char *)W(scratch, 0x2F4);
                dx = S(actor, 2) - S(ball, 2);
                dz = S(actor, 6) - S(ball, 6);
                if (dx * dx + dz * dz >= 0x4000) {
                    if (B(scratch, 0x29B) == 0) {
                        func_8005BA78(actor, 2, B(actor, 0x390));
                        break;
                    }
                }
            }
        }
        B(actor, 0x397) = 4;
        break;

    case 4:
        if (H(actor, 0x38E) != 1) {
            func_8005BA30(actor, 1, func_8001E6B8(0x15) & 0xFF);
        }
        func_8005BD1C(actor);
        if (B(scratch, 0x29A) == 1) {
            B(actor, 0x396) = 0;
            B(actor, 0x397) = 0;
        } else if (B(scratch, 0x29B) != 0) {
            B(actor, 0x396) = 0;
        }
        break;

    case 5:
        if (D_800E7658 < 0x10) {
            H(scratch, 0x294) = 0;
            break;
        }
        H(scratch, 0x294) = (unsigned short)(H(scratch, 0x294) + 1);
        func_8005C7EC();

        if (B(actor, 0x39A) == 0) {
            unsigned short input = H(actor, 0x3D4);
            if (input & 0x8000) {
                B(actor, 0x3AA) = (unsigned char)(B(actor, 0x3AA) + 4);
            } else if (input & 0x2000) {
                B(actor, 0x3AA) = (unsigned char)(B(actor, 0x3AA) - 4);
            }
        } else {
            ball = (unsigned char *)W(scratch, 0x2F4);
            ball_x_atk = S(ball, 2);
            if (B(actor, 0x399) != 0) {
                ball_x_atk = -ball_x_atk;
            }
            if (H(scratch, 0x294) >= 0x1FU) {
                if ((short)ball_x_atk < 0x23E2 || func_8001E6B8(4) != 0) {
                    H(actor, 0x3D6) = H(scratch, 0x16 + B(actor, 0x398) * 2);
                } else {
                    H(actor, 0x3D6) = H(scratch, 0x1A + B(actor, 0x398) * 2);
                }
            }
            if ((unsigned short)(ball_x_atk - 0xCC8) < 0x171AU) {
                B(actor, 0x3AA) = (unsigned char)func_8005B7CC((B(actor, 0x399) << 7) & 0x80, B(actor, 0x3AA), 1);
            }
        }

        heading = func_8005B7CC(B(actor, 0x3AA), B(actor, 0x3AB), 0x28);
        B(actor, 0x3AA) = (unsigned char)heading;
        turn = 0x1C0 - (heading & 0xFF);
        S(actor, 0x26) = (short)((turn % 256) * 16);

        side_off = B(actor, 0x398) * 2;
        mask = H(actor, 0x3D6);
        if (H(scratch, 0x16 + side_off) & mask) {
            S(actor, 0x3C2) = 0;
        } else if (H(scratch, 0x1A + side_off) & mask) {
            S(actor, 0x3C2) = 1;
        } else {
            break;
        }

        func_80050510(0);
        if (B(actor, 0x39A) == 0) {
            B(scratch, 0x2F2) = 3;
            func_80051A84(scratch + 0x1F0);
            for (i = 0; i < 8; i++) {
                W(scratch, 0x210 + i * 4) = W(scratch, 0x1F0 + i * 4);
            }
            B(scratch, 0x309) = 2;
        }
        B(scratch, 0x2C0) = 1;
        B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
        break;

    case 6:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0xB) {
            if (S(actor, 0x3C2) == 0) {
                power = func_8001E6B8(4) + 0x80;
                lift = 2;
            } else {
                power = func_8001E6B8(8) + 0x80;
                lift = func_8001E6B8(3) + 0x16;
            }
            func_8004B0F0(actor, B(actor, 0x3AA), power, lift);
            B(actor, 0x3D1) = 0;
            W(scratch, 0x2B0) = 6;
            func_8004B278();
            func_80049A94(actor, 0, 1, S(scratch, 0x2B4), S(scratch, 0x2B6));
            H(scratch, 0x31C) = 4;
            func_80076D44();
        }
        func_8005B864(actor, ((D_800CA58C[B(actor, 0x390)] << 5) / 20) & 0xFFFF);
        if (B(actor, 0x390) == 0) {
            func_8005BA30(actor, 2, 0);
            W(actor, 0x3A0) = 0x30;
            B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
        }
        break;

    case 7:
        func_8005BD1C(actor);
        if (B(actor, 0x390) >= 0xA) {
            func_8005BA30(actor, 1, 0);
            W(actor, 0x3A0) = 0;
            B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
        }
        heading = func_8005B36C(S(actor, 2), S(actor, 6), S(actor, 2), 0) & 0xFF;
        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(heading, B(actor, 0x3AA), 0xC);
        func_8005B864(actor, H(actor, 0x3A0));
        break;

    case 8:
        func_8005BD1C(actor);
        if (B(actor, 0x38D) != B(scratch, 0x2AC)) {
            B(actor, 0x396) = 0;
            B(actor, 0x397) = 0;
        }
        break;
    }
}
