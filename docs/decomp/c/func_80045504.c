/*
 * func_80045504
 *
 * Purpose:
 *   Player state-transition dispatcher during match play. Given the actor's
 *   current state (arg1, 0..20), queries action commands via func_80045064 and
 *   ball/possession/heading/distance checks, transitioning the actor's state
 *   (+0x396) and substate (+0x397) or triggering ball touch/dribble handlers.
 *
 * Arguments:
 *   actor - pointer to 1000-byte actor struct
 *   state - current actor state byte (0..20 handled; >=21 returns 0)
 *
 * Return value:
 *   1 if a state transition or action was triggered (or handled in-place),
 *   0 otherwise. (Cases 13 and 14 are no-ops; see UNCLEAR note.)
 *
 * Actor fields read:
 *   +0x02 (s16) x, +0x06 (s16) z, +0x38D (u8) actor id,
 *   +0x38E (u16) current anim id, +0x390 (u8) anim frame,
 *   +0x392 (u8) team slot, +0x396 (u8) state, +0x397 (u8) substate,
 *   +0x398 (u8) side, +0x399 (u8) attack direction flag,
 *   +0x39A (u8) AI/CPU control flag, +0x3A0 (u32) speed,
 *   +0x3A4 (u8) move heading, +0x3AA (u8) body heading,
 *   +0x3AB (u8) target heading, +0x3B0 (u8), +0x3B1 (u8), +0x3B2 (u8),
 *   +0x3B3 (u8) sprint speed cap, +0x3C6 (s16/u16), +0x3CC (s16/u16),
 *   +0x3D0 (u8) action lock/timer
 *
 * Actor fields written:
 *   +0x396 (u8) state, +0x397 (u8) substate, +0x3A0 (u32) speed,
 *   +0x3A4 (u8) move heading, +0x3AA (u8) body heading, +0x3AC (u8) mirror/foot,
 *   +0x3B0 (u8), +0x3B1 (u8), +0x3B2 (u8), +0x3C2 (u16),
 *   +0x3C6 (s16/u16), +0x3C8 (u16), +0x3CC (s16/u16),
 *   +0x3D0 (u8), +0x3D1 (u8)
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x36/+0x66 (s16) target x/z, +0x4E (s16) predicted ball height,
 *   +0x50 (s16) predicted ball height, +0x28C (u8) d-pad direction index,
 *   +0x29A (u8) match phase, +0x2A8 (u8) last-touch actor,
 *   +0x2A9 (u8) ball owner, +0x2AA (u8) previous owner,
 *   +0x2B0 (u32) ball event code, +0x2B8 (s16) sentinel (0x7FFF),
 *   +0x2CF[side] (u8) possession flags, +0x2F4 (ptr) ball actor,
 *   +0x2FC (u8) play-stoppage flag, +0x31C (s16) ball timer,
 *   +0x31E/+0x31F[atk_dir*2] (u8) marked/closest actor ids
 *
 * Scratchpad / ball fields written:
 *   Scratchpad +0x2A8 (u8), +0x2A9 (u8), +0x332 (u8)
 *   Ball +0x396 (u8) state = 3, Ball +0x3B0 (u8) = 0
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x64000 (409600 = 640^2) -> 2.56 m radius squared
 *   0x40000 (262144 = 512^2) -> 2.048 m radius squared
 *   0x100000 (1048576 = 1024^2) -> 4.096 m radius squared
 *   0xC3FF (50175, ~224^2) -> ~0.896 m radius squared
 *   0x6400 (25600 = 160^2) -> 0.64 m radius squared
 *   -0x120 (-288) -> -1.152 m height (upward)
 *   -0x73 (-115), -0x74 (-116) -> -0.46 m / -0.464 m height (upward)
 *   -0x5F (-95) -> -0.38 m height (upward)
 *   -0x1BE1 (-7137) -> -28.55 m x-coordinate
 *   0x32A0 (12960) -> 51.84 m x-coordinate
 *   0x31C < 8 / < 5 -> 0.27 s / 0.17 s since last ball touch
 *   Angle units: 0..255 = 360 deg (0x20 = 45 deg, 0x10 = 22.5 deg, 0x08 = 11.25 deg)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800C6B74[];
extern unsigned char D_8010C330[];

extern int func_80031024(unsigned char *actor);
extern int func_80031268(unsigned char *actor);
extern int func_800434D4(unsigned char *actor, unsigned int angle);
extern unsigned int func_80045064(unsigned char *actor, unsigned int flag);
extern void func_80046F8C(unsigned char *actor);
extern void func_800484F8(int x, int z, int radius, int arg3);
extern unsigned int func_80049398(unsigned char *actor, unsigned int fallback, unsigned int dist_sq_limit);
extern unsigned int func_800494C0(unsigned char *actor);
extern void func_8004A114(unsigned char *actor);
extern int func_80052E68(unsigned char *actor);
extern int func_80053EB8(unsigned char *actor);
extern void func_80054A1C(unsigned char *actor);
extern int func_80054D2C(unsigned char *actor);
extern int func_800551C8(unsigned char *actor);
extern int func_80055700(unsigned char *actor);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern void func_8005BA30(unsigned char *actor, unsigned int anim, unsigned int frame);
extern void func_8005BA78(unsigned char *actor, unsigned int anim, unsigned int frame);

static unsigned int angle_diff_u8(unsigned int a, unsigned int b)
{
    unsigned int diff = (a - b) & 0xFF;
    if (diff >= 0x81U) {
        diff = (b - a) & 0xFF;
    }
    return diff;
}

int func_80045504(unsigned char *actor, unsigned char state)
{
    unsigned char cmd;
    unsigned int heading;
    unsigned int diff;
    unsigned char pad_dir;
    unsigned char *ball;
    int dx;
    int dz;
    int dist_sq;
    int vy;

    if (state >= 21U) {
        return 0;
    }

    switch (state) {
    case 0:
        cmd = (unsigned char)func_80045064(actor, func_80054D2C(actor) & 0xFF);
        switch (cmd) {
        case 3:
        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
        case 12:
        case 13:
        case 15:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
        case 41:
        case 42:
            B(actor, 0x396) = cmd;
            B(actor, 0x397) = 0;
            return 1;
        default:
            break;
        }
        if (B(actor, 0x39A) != 0 && B(actor, 0x3D0) != 0) {
            return 0;
        }
        pad_dir = B(Scratchpad, 0x28C);
        if (pad_dir == 0) {
            return 0;
        }
        heading = D_800C6B74[pad_dir];
        if (B(actor, 0x38D) == B(Scratchpad, 0x2A8)) {
            if (angle_diff_u8(heading, B(actor, 0x3AA)) >= 0x21U) { /* > 45 deg */
                if (B(actor, 0x39A) == 0) {
                    func_80046F8C(actor);
                    return 1;
                }
                if (angle_diff_u8(func_80049398(actor, heading & 0xFF, 0x64000U), heading) < 0x71U) {
                    func_80046F8C(actor);
                    return 1;
                }
                return 1;
            }
        }
        if (B(actor, 0x39A) == 0 && B(Scratchpad, 0x2A9) != 0 && B(Scratchpad, 0x2CF + B(actor, 0x398)) == 1) {
            B(actor, 0x3A4) = (unsigned char)heading;
        } else {
            B(actor, 0x3A4) = B(actor, 0x3AA);
        }
        func_8005BA30(actor, 0x26, 0xF);
        B(actor, 0x396) = 1;
        B(actor, 0x397) = 0;
        return 1;

    case 1:
        cmd = (unsigned char)func_80045064(actor, func_80054D2C(actor) & 0xFF);
        switch (cmd) {
        case 3:
            if (angle_diff_u8(B(actor, 0x3AB), B(actor, 0x3AA)) >= 0x20U) {
                return 0;
            }
            if (angle_diff_u8(B(actor, 0x3AB), B(actor, 0x3A4)) >= 0x20U) {
                return 0;
            }
            if (W(actor, 0x3A0) < 0x28U) {
                return 0;
            }
            B(actor, 0x396) = cmd;
            B(actor, 0x397) = 0;
            return 1;
        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
        case 12:
        case 13:
        case 15:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
        case 41:
        case 42:
            B(actor, 0x396) = cmd;
            B(actor, 0x397) = 0;
            return 1;
        default:
            break;
        }
        pad_dir = B(Scratchpad, 0x28C);
        if (pad_dir != 0) {
            if (B(Scratchpad, 0x2A9) == 0) {
                if (S(Scratchpad, 0x4E) < -0x120) { /* < -1.152 m */
                    B(actor, 0x397) = 4;
                    return 1;
                }
                vy = SW(W(Scratchpad, 0x2F4), 0xC);
                if (vy < 0) {
                    vy = -vy;
                }
                if (vy >= 0x2E00) {
                    B(actor, 0x397) = 4;
                    return 1;
                }
            }
            if (B(actor, 0x38D) != B(Scratchpad, 0x2A8)) {
                return 0;
            }
            heading = D_800C6B74[pad_dir];
            if (angle_diff_u8(heading, B(actor, 0x3AA)) < 0x10U) { /* < 22.5 deg */
                return 0;
            }
            func_80046F8C(actor);
            return 1;
        }
        if (B(actor, 0x38D) != B(Scratchpad, 0x2A9)) {
            B(actor, 0x397) = 3;
            return 1;
        }
        if (B(Scratchpad, 0x2A8) != 0) {
            B(actor, 0x397) = 3;
            return 1;
        }
        B(actor, 0x397) = 2;
        return 1;

    case 2:
        cmd = (unsigned char)func_80045064(actor, 1);
        if ((cmd >= 4 && cmd < 7) || (cmd >= 8 && cmd < 11)) {
            B(actor, 0x3D1) = 2;
            B(actor, 0x396) = cmd;
            B(actor, 0x397) = 0;
            B(actor, 0x3A4) = B(actor, 0x3AA);
            return 1;
        }
        if (B(Scratchpad, 0x29A) == 1 && B(Scratchpad, 0x2FC) == 0) {
            return 0;
        }
        B(actor, 0x396) = 0x18;
        B(actor, 0x397) = 0;
        return 1;

    case 3:
        cmd = (unsigned char)func_80045064(actor, 1);
        if (angle_diff_u8(B(actor, 0x3AB), B(actor, 0x3AA)) < 0x60U) {
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            heading = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));
            diff = angle_diff_u8(heading, B(actor, 0x3AA));
        } else {
            diff = 0xFF;
        }
        if (S(actor, 0x3C6) == 0xFF) {
            if ((cmd >= 3 && cmd < 7) || (cmd >= 8 && cmd < 11)) {
                if ((diff & 0xFF) < 9U) {
                    B(actor, 0x396) = cmd;
                    B(actor, 0x397) = 0;
                    return 1;
                }
                S(actor, 0x3C6) = (signed short)cmd;
            }
        }
        if (S(actor, 0x3C6) == 0xFF || (diff & 0xFF) >= 9U) {
            return 0;
        }
        B(actor, 0x396) = (unsigned char)H(actor, 0x3C6);
        B(actor, 0x397) = 0;
        return 1;

    case 4:
        cmd = (unsigned char)func_80045064(actor, func_80054D2C(actor) & 0xFF);
        switch (cmd) {
        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
        case 12:
        case 13:
        case 15:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
        case 41:
        case 42:
            B(actor, 0x396) = cmd;
            B(actor, 0x397) = 0;
            return 1;
        default:
            return 0;
        }

    case 5:
        cmd = (unsigned char)func_80045064(actor, 1);
        if ((cmd >= 4 && cmd < 7) || (cmd >= 8 && cmd < 11)) {
            B(actor, 0x396) = cmd;
            B(actor, 0x397) = 0;
            return 1;
        }
        if (B(actor, 0x3D0) != 0) {
            return 0;
        }
        func_800484F8(S(actor, 2), S(actor, 6), 0x30, 0);
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        if (S(ball, 0x3BE) < -0x74) {
            return 0;
        }
        if (B(actor, 0x38D) == B(Scratchpad, 0x2A9) && B(Scratchpad, 0x2A8) == 0) {
            B(actor, 0x397) = 0x50;
            return 0;
        }
        if (B(actor, 0x39A) != 0) {
            func_8005BA30(actor, 0x34, 0xF);
            B(actor, 0x396) = 1;
            B(actor, 0x397) = 0;
            return 1;
        }
        pad_dir = B(Scratchpad, 0x28C);
        if (pad_dir == 0) {
            if (B(actor, 0x390) != 8) {
                return 0;
            }
            vy = SW(W(Scratchpad, 0x2F4), 0xC);
            if (vy < 0) {
                vy = -vy;
            }
            if (vy >= 0x8A0) {
                return 0;
            }
            B(Scratchpad, 0x332) = 1;
            B(actor, 0x396) = 0;
            B(actor, 0x397) = 0;
            return 1;
        }
        heading = D_800C6B74[pad_dir];
        if (B(actor, 0x38D) == B(Scratchpad, 0x2A8)) {
            if (angle_diff_u8(heading, B(actor, 0x3AA)) >= 0x21U) {
                if (B(actor, 0x39A) == 0 ||
                    angle_diff_u8(func_80049398(actor, heading & 0xFF, 0x64000U), heading) < 0x71U) {
                    W(actor, 0x3A0) = 0;
                    func_80046F8C(actor);
                    return 1;
                }
            }
        }
        B(Scratchpad, 0x332) = 1;
        func_8005BA30(actor, 0x34, 0xF);
        B(actor, 0x396) = 1;
        B(actor, 0x397) = 0;
        return 1;

    case 6:
        cmd = (unsigned char)func_80045064(actor, 1);
        switch (cmd) {
        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
            B(actor, 0x3B2) = cmd;
            break;
        case 10:
            B(actor, 0x3B1) = 1;
            B(actor, 0x3B2) = cmd;
            break;
        case 13:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
        case 41:
        case 42:
            B(actor, 0x396) = cmd;
            B(actor, 0x397) = 0;
            return 1;
        default:
            break;
        }
        dx = S(actor, 2) - S(Scratchpad, 0x36);
        dz = S(actor, 6) - S(Scratchpad, 0x66);
        dist_sq = dx * dx + dz * dz;
        if (dist_sq <= 0xC3FF && func_800494C0(actor) <= 0xFFFFU) { /* <= 0.896 m */
            B(actor, 0x3D0) = 4;
            W(actor, 0x3A0) = 0;
            H(actor, 0x3C6) = H(actor, 2);
            H(actor, 0x3C8) = H(actor, 6);
            func_8005BA30(actor, 0x58, 0);
            B(actor, 0x396) = 0;
            B(actor, 0x397) = 4;
            return 1;
        }
        if (B(actor, 0x3D0) != 0) {
            return 0;
        }
        if (B(actor, 0x3B2) != 0xFF) {
            B(actor, 0x396) = B(actor, 0x3B2);
            B(actor, 0x397) = 0;
            return 1;
        }
        if (B(actor, 0x39A) != 0 || B(Scratchpad, 0x28C) != 0) {
            unsigned int cap = B(actor, 0x3B3);
            unsigned int spd;
            func_8005BA30(actor, 0x34, 0xF);
            spd = W(actor, 0x3A0) + ((cap / 6U) & 0xFF);
            W(actor, 0x3A0) = spd;
            if (spd > cap) {
                W(actor, 0x3A0) = cap;
            }
            B(actor, 0x396) = 1;
            B(actor, 0x397) = 2;
            return 1;
        }
        if (S(Scratchpad, 0x50) < -0x73) { /* < -0.46 m */
            return 0;
        }
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        dx = S(actor, 2) - S(ball, 2);
        dz = S(actor, 6) - S(ball, 6);
        if (dx * dx + dz * dz < 0x6400) { /* < 0.64 m */
            B(ball, 0x396) = 3;
            B(Scratchpad, 0x2A9) = B(actor, 0x38D);
            B(Scratchpad, 0x2A8) = B(actor, 0x38D);
            if (H(actor, 0x38E) == 0x15) {
                unsigned int idx = B(actor, 0x398) * 264 + B(actor, 0x392) * 12;
                unsigned int flags = W(D_8010C330, idx);
                func_8005BA30(actor, 0x16, 0);
                B(actor, 0x3D0) = 4;
                B(actor, 0x396) = 0xE;
                B(actor, 0x3D1) = 0;
                B(actor, 0x397) = 0x10;
                B(actor, 0x3AC) = (flags >> 4) & 1;
                return 1;
            }
            func_8005BA30(actor, 1, 0);
            B(actor, 0x396) = 0;
            B(actor, 0x397) = 0;
            return 1;
        }
        B(actor, 0x396) = 0xE;
        B(actor, 0x397) = 0x30;
        return 1;

    case 7:
        if (B(actor, 0x396) != 0xB) {
            return 0;
        }
        if (W(Scratchpad, 0x2B0) != 1 && B(actor, 0x38D) != B(Scratchpad, 0x2A9)) {
            B(actor, 0x3D1) = 0;
            B(actor, 0x396) = 0x18;
            B(actor, 0x397) = 0;
            return 1;
        }
        if (S(actor, 0x3CC) != 0xFF) {
            heading = func_8005B36C(S(actor, 2), S(actor, 6), S(Scratchpad, 0x36), S(Scratchpad, 0x66));
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            if (angle_diff_u8(B(actor, 0x399) << 7, B(ball, 0x3A4)) < 0x31U) {
                diff = angle_diff_u8(heading, B(actor, 0x3AA));
            } else {
                diff = (heading - B(ball, 0x3A4) - 0x80) & 0xFF;
                if (diff >= 0x81U) {
                    diff = (-diff) & 0xFF;
                }
            }
            if ((diff & 0xFF) < 0x21U) {
                if (S(actor, 0x3CC) == 4) {
                    B(actor, 0x3D1) = 2;
                    B(actor, 0x396) = 4;
                    H(actor, 0x3C2) = 0;
                    B(actor, 0x3B0) = 0;
                    B(actor, 0x3B1) = 0;
                    B(actor, 0x397) = 1;
                    return 1;
                }
                B(actor, 0x396) = (unsigned char)S(actor, 0x3CC);
                B(actor, 0x397) = 0;
                return 1;
            }
            cmd = 0xFF;
        } else {
            cmd = (unsigned char)func_80045064(actor, 1);
        }
        switch (cmd) {
        case 4:
            H(actor, 0x3C6) = 4;
            /* fall through */
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
            if (B(actor, 0x38D) != B(Scratchpad, 0x2A9) && S(Scratchpad, 0x31C) < 8) {
                return 0;
            }
            H(actor, 0x3CC) = cmd;
            return 1;
        default:
            if (B(actor, 0x38D) == B(Scratchpad, 0x2A9)) {
                func_80031024(actor);
                return 1;
            }
            if ((func_80052E68(actor) & 0xFF) == 0) {
                return 0;
            }
            B(actor, 0x3D1) = 0;
            B(actor, 0x396) = 0x18;
            B(actor, 0x397) = 0;
            return 1;
        }

    case 8:
        cmd = (unsigned char)func_80045064(actor, func_80054D2C(actor) & 0xFF);
        switch (cmd) {
        case 3:
        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
        case 13:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
        case 41:
        case 42:
            B(actor, 0x396) = cmd;
            B(actor, 0x397) = 0;
            return 1;
        default:
            return 0;
        }

    case 9:
        if (B(actor, 0x3D0) == 0) {
            func_8004A114(actor);
        }
        cmd = (unsigned char)func_80045064(actor, func_80054D2C(actor) & 0xFF);
        switch (cmd) {
        case 3:
            if (H(actor, 0x38E) == 0xD) {
                W(actor, 0x3A0) = 0;
            }
            /* fall through */
        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
        case 12:
        case 13:
        case 15:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
        case 41:
        case 42:
            B(actor, 0x396) = cmd;
            B(actor, 0x397) = 0;
            return 0;
        default:
            break;
        }
        if (func_80055700(actor) != 0 &&
            B(Scratchpad, 0x29A) == 1 &&
            B(Scratchpad, 0x2FC) == 0 &&
            (B(actor, 0x3D0) != 0 || B(Scratchpad, 0x28C) != 0)) {
            if (B(actor, 0x38D) != B(Scratchpad, 0x2A9)) {
                return 0;
            }
            func_80054A1C(actor);
            B(actor, 0x397) = 1;
            return 0;
        }
        B(actor, 0x3B0) = 0;
        B(actor, 0x397) = 3;
        return 0;

    case 10:
        cmd = (unsigned char)func_80045064(actor, 1);
        if ((cmd >= 4 && cmd < 7) || (cmd >= 8 && cmd < 11)) {
            func_8005BA30(actor, 0x26, 5);
            B(actor, 0x396) = cmd;
            B(actor, 0x397) = 0;
            return 1;
        }
        if (B(Scratchpad, 0x29A) != 1 || B(Scratchpad, 0x2FC) != 0) {
            B(actor, 0x396) = 1;
            B(actor, 0x397) = 0;
            return 1;
        }
        if (B(actor, 0x390) != 0 && S(Scratchpad, 0x4E) < -0x5F) { /* < -0.38 m */
            return 0;
        }
        func_8005BA30(actor, 4, 4);
        B(actor, 0x396) = 3;
        B(actor, 0x397) = 2;
        return 1;

    case 11:
        cmd = (unsigned char)func_80045064(actor, func_80054D2C(actor) & 0xFF);
        switch (cmd) {
        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
        case 13:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
        case 41:
        case 42:
            B(actor, 0x396) = cmd;
            B(actor, 0x397) = 0;
            return 1;
        default:
            break;
        }
        {
            unsigned char actor_id = B(actor, 0x38D);
            unsigned char owner = B(Scratchpad, 0x2A9);
            unsigned char *pair;

            if (actor_id == B(Scratchpad, 0x2A8)) {
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 0;
                return 1;
            }
            if (actor_id == owner) {
                B(actor, 0x396) = 3;
                B(actor, 0x397) = 2;
                return 1;
            }
            if (B(Scratchpad, 0x2CF + B(actor, 0x398)) == 0 && owner != 0) {
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 0;
                return 1;
            }
            if (B(Scratchpad, 0x29A) != 1 || B(Scratchpad, 0x2FC) != 0) {
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 0;
                return 1;
            }
            pair = Scratchpad + (B(actor, 0x399) << 1);
            if (actor_id != B(pair, 0x31E) && actor_id != B(pair, 0x31F)) {
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 0;
                return 1;
            }
        }
        if (B(actor, 0x39A) != 2) {
            if (func_80055700(actor) == 0) {
                if (func_800551C8(actor) != 0) {
                    return 0;
                }
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                dx = S(actor, 2) - S(ball, 2);
                dz = S(actor, 6) - S(ball, 6);
                if (dx * dx + dz * dz > 0x100000) { /* > 4.096 m */
                    B(actor, 0x396) = 1;
                    B(actor, 0x397) = 0;
                    return 1;
                }
            } else {
                if (B(actor, 0x39A) == 0 && B(actor, 0x3B0) == 0) {
                    B(actor, 0x396) = 1;
                    B(actor, 0x397) = 0;
                    return 1;
                }
            }
            if ((func_80053EB8(actor) & 0xFF) == 0) {
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 0;
                return 1;
            }
        }
        {
            int bx = S((unsigned char *)W(Scratchpad, 0x2F4), 2);
            if (B(actor, 0x399) != 0) {
                bx = -bx;
            }
            if (bx < -0x1BE1) { /* < -28.55 m */
                return 0;
            }
        }
        if (B(actor, 0x3B1) == B(Scratchpad, 0x2A9) && B(actor, 0x3B0) != 0) {
            return 0;
        }
        B(actor, 0x396) = 1;
        B(actor, 0x397) = 0;
        return 1;

    case 12:
        if (W(Scratchpad, 0x2B0) != 1 && B(actor, 0x38D) != B(Scratchpad, 0x2A9)) {
            B(actor, 0x3D1) = 0;
            B(actor, 0x396) = 0x18;
            B(actor, 0x397) = 0;
            return 1;
        }
        {
            int ax = S(actor, 2);
            if (B(actor, 0x399) != 0) {
                ax = -ax;
            }
            if (ax >= 0x32A0) { /* >= 51.84 m */
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 0;
                return 1;
            }
        }
        if (S(actor, 0x3CC) != 0xFF) {
            heading = func_8005B36C(S(actor, 2), S(actor, 6), S(Scratchpad, 0x36), S(Scratchpad, 0x66));
            if (angle_diff_u8(heading, B(actor, 0x3AA)) < 0x10U && B(actor, 0x3D0) == 0) {
                if (S(actor, 0x3CC) == 4) {
                    B(actor, 0x3D1) = 2;
                    B(actor, 0x396) = 4;
                    H(actor, 0x3C2) = 0;
                    B(actor, 0x3B0) = 0;
                    B(actor, 0x3B1) = 0;
                    B(actor, 0x397) = 1;
                    return 1;
                }
                B(actor, 0x396) = (unsigned char)S(actor, 0x3CC);
                B(actor, 0x397) = 0;
                return 1;
            }
            cmd = 0xFF;
        } else {
            cmd = (unsigned char)func_80045064(actor, 1);
        }
        switch (cmd) {
        case 4:
            H(actor, 0x3C6) = 4;
            /* fall through */
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
            H(actor, 0x3CC) = cmd;
            return 1;
        default:
            if (B(actor, 0x38D) == B(Scratchpad, 0x2A8)) {
                func_80031268(actor);
                return 1;
            }
            if ((func_80052E68(actor) & 0xFF) == 0) {
                return 0;
            }
            B(actor, 0x3D1) = 0;
            B(actor, 0x396) = 0x18;
            B(actor, 0x397) = 0;
            return 1;
        }

    case 13:
    case 14:
        /* UNCLEAR: Jump table entries for state 13 and 14 point directly to the
         * epilogue (.L80046F6C) without setting $v0 = 0 (likely missing return
         * value in original C switch). */
        return 0;

    case 15:
        cmd = (unsigned char)func_80045064(actor, 0);
        switch (cmd) {
        case 3:
            if (S(Scratchpad, 0x2B8) != 0x7FFF) {
                return 0;
            }
            B(actor, 0x396) = cmd;
            B(actor, 0x397) = 0;
            return 1;
        case 4:
        case 13:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
        case 41:
        case 42:
            B(actor, 0x396) = cmd;
            B(actor, 0x397) = 0;
            return 1;
        default:
            break;
        }
        if (S(Scratchpad, 0x4E) < -0x120) {
            if (B(Scratchpad, 0x2A9) == 0) {
                return 0;
            }
        } else {
            vy = SW(W(Scratchpad, 0x2F4), 0xC);
            if (vy < 0) {
                vy = -vy;
            }
            if (vy >= 0x2E00 && B(Scratchpad, 0x2A9) == 0) {
                return 0;
            }
        }
        func_8005BA78(actor, 0x26, B(actor, 0x390));
        B(actor, 0x396) = 1;
        B(actor, 0x397) = 1;
        return 1;

    case 16:
        cmd = (unsigned char)func_80045064(actor, 1);
        switch (cmd) {
        case 3:
            if (B(Scratchpad, 0x2A9) == 0) {
                B(actor, 0x396) = 3;
                B(actor, 0x397) = 2;
                return 0;
            }
            /* fall through */
        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            B(ball, 0x3B0) = 0;
            B(actor, 0x396) = cmd;
            B(actor, 0x397) = 0;
            return 0;
        default:
            break;
        }
        if (B(Scratchpad, 0x29A) != 1 || B(Scratchpad, 0x2FC) != 0) {
            B(actor, 0x397) = 1;
            return 0;
        }
        pad_dir = B(Scratchpad, 0x28C);
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        if (pad_dir == 0) {
            dx = S(actor, 2) - S(ball, 2);
            dz = S(actor, 6) - S(ball, 6);
            if (dx * dx + dz * dz > 0xC3FF) { /* > 0.896 m */
                return 0;
            }
            if (S(ball, 4) < -0x73) { /* < -0.46 m */
                return 0;
            }
            B(Scratchpad, 0x2A8) = B(actor, 0x38D);
            B(ball, 0x396) = 3;
            B(actor, 0x397) = 3;
            return 0;
        }
        if (B(actor, 0x3D0) == 0) {
            dx = S(actor, 2) - S(ball, 2);
            dz = S(actor, 6) - S(ball, 6);
            if (dx * dx + dz * dz <= 0xC3FF &&
                angle_diff_u8(B(ball, 0x3A4), B(actor, 0x3AB)) < 8U &&
                S(ball, 4) >= -0x73) {
                heading = D_800C6B74[pad_dir];
                if (angle_diff_u8(heading, B(actor, 0x3AA)) >= 0x10U &&
                    angle_diff_u8(func_80049398(actor, heading & 0xFF, 0x40000U), heading) < 0x61U) {
                    B(Scratchpad, 0x2A9) = B(actor, 0x38D);
                    B(Scratchpad, 0x2A8) = B(actor, 0x38D);
                    B(ball, 0x396) = 3;
                    func_80046F8C(actor);
                    return 0;
                }
            }
        }
        {
            unsigned char owner = B(Scratchpad, 0x2A9);
            unsigned char actor_id = B(actor, 0x38D);
            if (owner == 0) {
                return 0;
            }
            if (actor_id != B(Scratchpad, 0x2A8) && actor_id == owner && B(Scratchpad, 0x29A) == 1) {
                return 0;
            }
            B(actor, 0x397) = 1;
            B(actor, 0x3AA) = (unsigned char)((B(actor, 0x3AA) + 0x10) & 0x1E0);
            return 0;
        }

    case 17:
        if ((func_80053EB8(actor) & 0xFF) == 0) {
            B(actor, 0x396) = 0xF;
            B(actor, 0x397) = 0;
            return 1;
        }
        cmd = (unsigned char)func_80045064(actor, func_80054D2C(actor) & 0xFF);
        switch (cmd) {
        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
            B(actor, 0x3D1) = 2;
            B(actor, 0x396) = cmd;
            B(actor, 0x397) = 0;
            return 1;
        case 13:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
        case 41:
        case 42:
            B(actor, 0x396) = cmd;
            B(actor, 0x397) = 0;
            return 1;
        default:
            break;
        }
        if (B(Scratchpad, 0x29A) != 1 || B(Scratchpad, 0x2FC) != 0) {
            B(actor, 0x396) = 0x18;
            B(actor, 0x397) = 0;
            return 1;
        }
        if ((func_80052E68(actor) & 0xFF) == 0 && S(Scratchpad, 0x31C) >= 5) {
            return 0;
        }
        {
            unsigned char actor_id = B(actor, 0x38D);
            if (actor_id == B(Scratchpad, 0x2A8)) {
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 0;
                return 1;
            }
            if (actor_id == B(Scratchpad, 0x2A9) || actor_id == B(Scratchpad, 0x2AA)) {
                B(actor, 0x396) = 3;
                B(actor, 0x397) = 2;
                return 1;
            }
            if ((unsigned char)(B(actor, 0x397) - 0x20) < 2U) {
                B(actor, 0x3D1) = 0;
                B(actor, 0x396) = 0x18;
                B(actor, 0x397) = 0;
                return 1;
            }
            B(actor, 0x396) = 1;
            B(actor, 0x397) = 0;
            return 1;
        }

    case 18:
        cmd = (unsigned char)func_80045064(actor, func_80054D2C(actor) & 0xFF);
        switch (cmd) {
        case 4:
        case 13:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
        case 41:
        case 42:
            B(actor, 0x396) = cmd;
            B(actor, 0x397) = 0;
            return 0;
        default:
            break;
        }
        if ((func_80052E68(actor) & 0xFF) == 0) {
            return 0;
        }
        if (B(actor, 0x38D) == B(Scratchpad, 0x2A9)) {
            B(actor, 0x396) = 0x1B;
            B(actor, 0x397) = 1;
            return 0;
        }
        B(actor, 0x396) = 0x18;
        B(actor, 0x397) = 0;
        if (B(Scratchpad, 0x29A) == 1 && B(Scratchpad, 0x2CF + B(actor, 0x398)) == 0) {
            B(actor, 0x3D0) = 6;
            B(actor, 0x397) = 2;
        }
        return 0;

    case 19:
        cmd = (unsigned char)func_80045064(actor, 0);
        switch (cmd) {
        case 4:
        case 8:
        case 9:
        case 13:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
        case 41:
        case 42:
            B(actor, 0x396) = cmd;
            B(actor, 0x397) = 0;
            return 1;
        default:
            break;
        }
        if ((func_80052E68(actor) & 0xFF) == 0) {
            unsigned int event = W(Scratchpad, 0x2B0);
            if (event == 3U || event == 4U) {
                return 0;
            }
        }
        B(actor, 0x396) = 0x24;
        B(actor, 0x397) = 0;
        return 1;

    case 20:
        cmd = (unsigned char)func_80045064(actor, func_80054D2C(actor) & 0xFF);
        switch (cmd) {
        case 3:
        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
        case 13:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
        case 41:
        case 42:
            B(actor, 0x396) = cmd;
            B(actor, 0x397) = 0;
            return 1;
        default:
            break;
        }
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        heading = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));
        if (func_800434D4(actor, heading & 0xFF) != 0) {
            return 1;
        }
        if (B(Scratchpad, 0x29A) != 1 || B(Scratchpad, 0x2FC) != 0) {
            B(actor, 0x396) = 1;
            B(actor, 0x397) = 0;
            return 1;
        }
        {
            unsigned char owner = B(Scratchpad, 0x2A9);
            unsigned char actor_id = B(actor, 0x38D);
            if (owner == 0) {
                return 0;
            }
            if (actor_id != B(Scratchpad, 0x2A8) && actor_id == owner) {
                return 0;
            }
            B(actor, 0x396) = 1;
            B(actor, 0x397) = 0;
            return 1;
        }
    }

    return 0;
}
