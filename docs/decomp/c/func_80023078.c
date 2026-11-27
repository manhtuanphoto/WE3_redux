/*
 * func_80023078 - Player ball-claim / shielding / jockeying substate machine.
 *
 * Purpose:
 *   Updates an actor's ball-claim/shielding state machine (substates 0..4), initializing
 *   speed/acceleration from player stats and stamina, orienting toward the ball or an
 *   approaching opponent, optionally triggering a pass/clearance or body-block, and claiming
 *   loose/low balls when in range.
 *
 * Arguments:
 *   actor (unsigned char*): player actor struct (1000 bytes).
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F80004E (s16, R): ball predicted Y height (-0x3F = -0.25 m threshold to claim).
 *   - 0x1F800240 / 0x1F800241 (u8, R): special/assigned actor IDs.
 *   - 0x1F80028C (u8, R): flag skipping opponent-check in substate 2 when 0x39A == 0.
 *   - 0x1F80029A (u8, R): match phase (1 = open play).
 *   - 0x1F8002A8 (u8, R/W): last-touch actor ID.
 *   - 0x1F8002A9 (u8, R/W): current ball owner (0 = free).
 *   - 0x1F8002B0 (u32, R): ball event code (1 = pass/loose).
 *   - 0x1F8002CF[2] (u8, R): team possession flags.
 *   - 0x1F8002F4 (u8*, R): pointer to ball actor.
 *   - 0x1F8002FC (u8, R): early-exit freeze/replay flag.
 *   - 0x1F80031C (s16, R): ball timer (frames since last touch).
 *   - 0x1F80031E / 0x1F800322 (u8[2][2], R): nearest/target player indices per attack dir.
 *
 * Actor fields reads/writes:
 *   - actor: +0x02/+0x04/+0x06 (s16 xyz, R/W), +0x08/+0x10 (s32 vx/vz, W=0), +0x26 (s16 rot, W),
 *            +0x38D (u8 id, R), +0x38E (u16 anim id, R), +0x390 (u8 anim frame, R),
 *            +0x392 (u8 team slot, R), +0x396 (u8 state, W), +0x397 (u8 substate, R/W),
 *            +0x398 (u8 side, R), +0x399 (u8 attack dir, R), +0x39A (u8 control/AI flag, R),
 *            +0x3A0 (u32 speed, W=0), +0x3A4 (u8 move heading, W), +0x3A7 (u8, W=1),
 *            +0x3AA (u8 body heading, R/W), +0x3AB (u8 target heading, W), +0x3AC (u8 mirror/foot, W),
 *            +0x3AE (s16 stamina, R/W), +0x3B0 (u8 timer, W), +0x3B4 (u8 max speed, R/W),
 *            +0x3B5 (u8 accel divisor, R/W), +0x3B8 (u8 stat*10, W),
 *            +0x3C2 (s16 pass target, W), +0x3C4 (s16, W=0), +0x3C6/+0x3C8 (s16 saved x/z, R/W),
 *            +0x3D0 (u8, R/W), +0x3D1 (u8, W=1).
 *   - target: +0x02/+0x06 (s16 x/z, R/W), +0x08/+0x10 (s32 vx/vz, R), +0x3A0 (u32 speed, R/W),
 *             +0x3A4 (u8 move heading, R).
 *   - ball:   +0x02/+0x06 (s16 x/z, R), +0x04 (s16 y, W=-0x20), +0x396 (u8 state, W=3).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0xFA0 (4000) = stamina threshold for full speed/accel.
 *   - 0x5A (90 frames = 3.00 s) = timer set when actor is already last-touch.
 *   - 0xC3FF (50175, ~224^2) -> 0.90 m squared XZ range to opponent in substate 2.
 *   - 0x6910 (26896 = 164^2) -> 0.66 m squared XZ range to push back moving opponent in substate 4.
 *   - 0x10000 (65536 = 256^2) -> 1.02 m squared XZ threshold in substate 4.
 *   - -0x3F (-63) = 0.25 m ball height limit; -0x20 (-32) = -0.13 m ball clamp height.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];
extern unsigned char D_8010C328[];
extern unsigned char D_8010C32B[];
extern unsigned char D_8010C330[];
extern unsigned char D_800C6AE4[];
extern unsigned char D_800C6BF0[];

extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern void func_8005B908(unsigned char *actor);
extern int func_8005B7CC(int target, int cur, int step);
extern void func_8005BD1C(unsigned char *actor);
extern int func_8001E6B8(int n);
extern void func_80067860(int atk_dir, int x, int z, int heading, int a4, int a5);
extern void func_800676C4(int atk_dir, int x, int z, int a3);
extern void func_80045504(unsigned char *actor, int a1);

void func_80023078(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *ball;
    unsigned char *target;
    unsigned int substate;
    int offset;
    unsigned int stat_word;
    int base_max_spd;
    int stamina;
    int angle, heading, rot;
    int opp_idx;
    int dx, dz;
    unsigned int dist_sq;
    int pass_target;

    substate = B(actor, 0x397);
    B(actor, 0x3A7) = 1;
    if (substate >= 5) {
        return;
    }

    switch (substate) {
    case 0:
        S(actor, 0x4) = 0;
        B(actor, 0x3AC) = 0;
        if (H(actor, 0x38E) != 1) {
            func_8005BA30(actor, 1, 0);
        }
        W(actor, 0x3A0) = 0;
        W(actor, 0x8) = 0;
        W(actor, 0x10) = 0;
        B(actor, 0x3D0) = 4;

        offset = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        stat_word = W(D_8010C328 + offset);
        base_max_spd = D_800C6AE4[(stat_word >> 20) & 0xF] + 0x35;
        B(actor, 0x3B4) = (unsigned char)base_max_spd;

        stamina = S(actor, 0x3AE);
        if (stamina <= 0) {
            S(actor, 0x3AE) = 0;
            B(actor, 0x3B5) = 0x1C;
            B(actor, 0x3B4) = (unsigned char)(B(actor, 0x3B4) - 4);
        } else if (stamina < 0xFA0) {
            B(actor, 0x3B4) = (unsigned char)(base_max_spd - ((0xFA0 - stamina) / 1000));
            B(actor, 0x3B5) = (unsigned char)(((0xFA0 - S(actor, 0x3AE)) / 400) + 0x12);
        } else {
            B(actor, 0x3B5) = 0x12;
        }

        offset = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        B(actor, 0x3B5) = (unsigned char)(B(actor, 0x3B5) - D_800C6BF0[(W(D_8010C330 + offset) & 0xF) * 2]);

        if (B(actor, 0x38D) == B(scratch, 0x2A8)) {
            offset = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
            B(actor, 0x3B0) = 0x5A;
            B(actor, 0x397) = 2;
            B(actor, 0x3AC) = (unsigned char)((W(D_8010C330 + offset) >> 4) & 1);
            func_80045504(actor, 0);
            return;
        }

        if (B(actor, 0x38D) != B(scratch, 0x2A9)) {
            B(actor, 0x3D1) = 1;
        }
        offset = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        B(actor, 0x3B8) = (unsigned char)((B(D_8010C32B, offset) & 0xF) * 10);
        ball = (unsigned char *)W(scratch, 0x2F4);
        B(actor, 0x3AB) = (unsigned char)func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));
        B(actor, 0x397) = 1;
        /* fall through */

    case 1:
        if (B(actor, 0x38D) != B(scratch, 0x2A8)) {
            if (B(scratch, 0x29A) != 1 || B(scratch, 0x2FC) != 0) {
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 5;
                func_8005B908(actor);
                return;
            }
        }
        ball = (unsigned char *)W(scratch, 0x2F4);
        angle = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6)) & 0xFF;
        heading = func_8005B7CC(angle, B(actor, 0x3AA), 0x10);
        B(actor, 0x3AA) = (unsigned char)heading;
        B(actor, 0x3A4) = (unsigned char)heading;
        rot = (0x1C0 - (int)B(actor, 0x3AA)) % 256;
        S(actor, 0x26) = (signed short)(rot << 4);

        if (B(actor, 0x39A) == 0 && W(scratch, 0x2B0) == 1 && B(scratch, 0x2CF + B(actor, 0x398)) == 1) {
            offset = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
            if (S(scratch, 0x31C) < (int)(10 - (W(D_8010C328 + offset) >> 28))) {
                return;
            }
        }

        if (B(actor, 0x38D) == B(scratch, 0x2A9)) {
            func_80045504(actor, 0);
            return;
        }
        if (B(actor, 0x39A) == 2) {
            B(actor, 0x396) = 7;
            B(actor, 0x397) = 0;
            return;
        }
        if (B(actor, 0x38D) == B(scratch, 0x240) || B(actor, 0x38D) == B(scratch, 0x241)) {
            func_80045504(actor, 0);
            return;
        }
        B(actor, 0x396) = 7;
        B(actor, 0x397) = 0;
        return;

    case 2:
        func_8005BD1C(actor);
        if (B(actor, 0x39A) == 0 && B(scratch, 0x28C) != 0) {
            B(actor, 0x3A4) = B(actor, 0x3AA);
            W(actor, 0x3A0) = 0;
            func_80045504(actor, 0);
            return;
        }

        opp_idx = B(scratch, 0x31E + ((B(actor, 0x399) ^ 1) << 1));
        target = D_800FF748 + (opp_idx % 24) * 1000;
        dx = S(actor, 2) - S(target, 2);
        dz = S(actor, 6) - S(target, 6);
        if (dx * dx + dz * dz > 0xC3FF) {
            func_80045504(actor, 0);
            return;
        }

        if (B(actor, 0x39A) == 0) {
            int diff;
            angle = func_8005B36C(S(target, 2), S(target, 6), S(actor, 2), S(actor, 6));
            diff = (B(actor, 0x3AA) - angle) & 0xFF;
            if ((unsigned int)diff < 0x81U) {
                angle = func_8005B36C(S(target, 2), S(target, 6), S(actor, 2), S(actor, 6));
                if (((B(actor, 0x3AA) - angle) & 0xFF) >= 0x30) {
                    B(actor, 0x3A4) = B(actor, 0x3AA);
                    W(actor, 0x3A0) = 0;
                    func_80045504(actor, 0);
                    return;
                }
            } else {
                angle = func_8005B36C(S(target, 2), S(target, 6), S(actor, 2), S(actor, 6));
                if (((angle - B(actor, 0x3AA)) & 0xFF) >= 0x30) {
                    B(actor, 0x3A4) = B(actor, 0x3AA);
                    W(actor, 0x3A0) = 0;
                    func_80045504(actor, 0);
                    return;
                }
            }
        }

        if (B(actor, 0x39A) != 0) {
            if (func_8001E6B8(2) != 0) {
                func_80067860(B(actor, 0x399), S(actor, 2), S(actor, 6), B(actor, 0x3AA), 0x38, 0x1000000);
                pass_target = B(scratch, 0x322 + (B(actor, 0x399) << 1));
                if ((pass_target & 0xFF) != 0xFF) {
                    S(actor, 0x3C2) = (signed short)pass_target;
                    S(actor, 0x3C4) = 0;
                    B(actor, 0x396) = 5;
                    B(actor, 0x397) = 0;
                    return;
                }
            }
        }

        func_8005BA30(actor, 0x58, 0);
        B(actor, 0x3D0) = 4;
        B(actor, 0x397) = 4;
        H(actor, 0x3C6) = H(actor, 2);
        H(actor, 0x3C8) = H(actor, 6);
        B(actor, 0x3A4) = B(actor, 0x3AA);
        W(actor, 0x3A0) = 0;
        func_80045504(actor, 0);
        return;

    case 3:
        if (H(actor, 0x38E) == 0x58) {
            int frame = B(actor, 0x390);
            if (frame == 0) {
                func_8005BA30(actor, 1, 0);
                B(actor, 0x397) = 2;
            } else {
                func_8005BA30(actor, 0x58, (frame - 1) & 0xFF);
            }
        } else {
            func_8005BD1C(actor);
        }
        func_80045504(actor, 0);
        return;

    case 4:
        ball = (unsigned char *)W(scratch, 0x2F4);
        func_800676C4(B(actor, 0x399) ^ 1, S(ball, 2), S(ball, 6), 0);
        opp_idx = B(scratch, 0x322 + ((B(actor, 0x399) ^ 1) << 1));
        target = D_800FF748 + (opp_idx % 24) * 1000;

        ball = (unsigned char *)W(scratch, 0x2F4);
        angle = func_8005B36C(
            S(ball, 2),
            S(ball, 6),
            (signed short)(H(target, 2) + W(target, 8)),
            (signed short)(H(target, 6) + W(target, 16))
        );
        heading = func_8005B7CC((angle + 0x80) & 0xFF, B(actor, 0x3AA), 8);
        B(actor, 0x3AA) = (unsigned char)heading;
        rot = (0x1C0 - (heading & 0xFF)) % 256;
        S(actor, 0x26) = (signed short)(rot << 4);

        dx = S(actor, 2) - S(target, 2);
        dz = S(actor, 6) - S(target, 6);
        H(actor, 6) = H(actor, 0x3C8);
        H(actor, 2) = H(actor, 0x3C6);
        dist_sq = (unsigned int)(dx * dx + dz * dz);

        if (dist_sq < 0x6910U && W(target, 0x3A0) != 0) {
            int t_dir = B(target, 0x3A4);
            int a_dir = B(actor, 0x3AA);
            unsigned int diff = (unsigned int)((t_dir - a_dir) & 0xFF);
            if ((diff < 0x81U && (int)diff < 0x10) ||
                (diff >= 0x81U && ((a_dir - t_dir) & 0xFF) < 0x10)) {
                H(target, 2) = (unsigned short)(H(target, 2) - W(target, 8));
                H(target, 6) = (unsigned short)(H(target, 6) - W(target, 16));
                W(target, 0x3A0) >>= 1;
            }
        }

        H(actor, 0x3C6) = H(actor, 2);
        H(actor, 0x3C8) = H(actor, 6);

        if (S(scratch, 0x4E) < -0x3F) {
            func_80045504(actor, 0);
            return;
        }

        ball = (unsigned char *)W(scratch, 0x2F4);
        S(ball, 4) = -0x20;
        ball = (unsigned char *)W(scratch, 0x2F4);
        B(scratch, 0x2A9) = B(actor, 0x38D);
        B(scratch, 0x2A8) = B(actor, 0x38D);
        B(ball, 0x396) = 3;

        if (H(actor, 0x38E) == 0x58) {
            func_8005BD1C(actor);
            if (B(actor, 0x390) == 0) {
                func_8005BA30(actor, 0x55, 0);
                B(actor, 0x3D0) = 1;
            }
        } else {
            if (B(actor, 0x390) != 8 || dist_sq > 0x10000U || B(actor, 0x39A) != 0) {
                func_8005BD1C(actor);
                if (B(actor, 0x39A) != 0) {
                    func_80067860(B(actor, 0x399), S(actor, 2), S(actor, 6), B(actor, 0x3AA), 0x30, 0x1000000);
                    pass_target = B(scratch, 0x322 + (B(actor, 0x399) << 1));
                    if ((pass_target & 0xFF) != 0xFF) {
                        S(actor, 0x3C2) = (signed short)pass_target;
                        S(actor, 0x3C4) = 0;
                        B(actor, 0x396) = 5;
                        B(actor, 0x397) = 0;
                        return;
                    }
                }
            }
        }

        if (H(actor, 0x38E) == 0x55 && B(actor, 0x390) == 0 && B(actor, 0x3D0) == 0) {
            func_8005BA30(actor, 0x58, 5);
            B(actor, 0x397) = 3;
            B(actor, 0x3AA) = (unsigned char)((B(actor, 0x3AA) + 0x10) & 0x1E0);
        }
        func_80045504(actor, 0);
        return;
    }
}
