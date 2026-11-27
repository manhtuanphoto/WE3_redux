/*
 * func_800930E0 - Goalkeeper diving-catch / punch-clear substate machine.
 *
 * Purpose:
 *   Executes the goalkeeper's dive/jump (substates 0..1) toward the intercept point
 *   at +0x3C2/+0x3C4/+0x3C6: initializes animation (0x61 or 0x72), horizontal and
 *   vertical velocities, then steps position per animation frame (1..15) and checks
 *   ball touch/catch/parry on frame 9 before returning to idle/recovery (0x81/0x82).
 *
 * Arguments:
 *   actor (unsigned char *): goalkeeper actor struct (1000 bytes).
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F8000F0/F2/F4 (s16, R): scratch XYZ (dive origin).
 *   - 0x1F8002A8 (u8, R): last-touch actor ID.
 *   - 0x1F8002F4 (u8 *, R): pointer to ball actor.
 *   - 0x1F80031C (s16, R/W): ball timer (+= 6 on touch in frame 9).
 *
 * Actor fields reads/writes:
 *   - actor: +0x02/+0x04/+0x06 (s16 xyz, R/W), +0x08/+0x0C/+0x10 (s32 vx/vy/vz, R/W),
 *            +0x26 (s16 model rotation, W), +0x38D (u8 id, R),
 *            +0x390 (u8 anim frame, R), +0x392 (u8 team slot, R),
 *            +0x396 (u8 state, R/W), +0x397 (u8 substate, R/W),
 *            +0x398 (u8 side, R), +0x3AA (u8 body heading, R/W),
 *            +0x3AB (u8 target heading, R), +0x3AC (u8 mirror/foot, R),
 *            +0x3C2/+0x3C4/+0x3C6 (s16 dive target xyz, R),
 *            +0x3CC (s16 end anim frame, R/W).
 *   - ball:  +0x0C (s32 vy * 736, R), +0x3A4 (u8 move heading, R).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x32 (50 raw/frame = 6.0 m/s) = base dive speed cap + (stat >> 1).
 *   - 0x17 / 0x16 (23 / 22 frames) = dive duration for state 0x8C vs other.
 *   - 0x100 (256 raw = 1.024 m) = touch radius passed to func_80094EEC.
 *   - 736 = ball vertical velocity scale factor (ball+0x0C / 736).
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_8010C32F[];

extern void func_8005C2AC(unsigned char *actor, int anim, int a2, int target_heading, int mirror);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern unsigned int func_800B1128(int val);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8001E694(int angle, int scale);
extern int func_8001E5A4(int angle, int scale);
extern void func_80094474(signed int *vy_ptr, int a1, int grav);
extern int func_8005B7CC(int target, int cur, int step);
extern void func_8005BD1C(unsigned char *actor);
extern int func_80094EEC(unsigned char *actor, int radius);
extern int func_80094DC4(unsigned char *actor);
extern int func_8001E6B8(int n);
extern void func_800951C4(unsigned char *actor, int heading, int speed, int a3);
extern void func_80095090(unsigned char *actor);
extern void func_80057968(unsigned char *actor);
extern void func_80094FF4(unsigned char *actor);
extern void func_8005778C(unsigned char *actor);
extern void func_8009462C(unsigned char *actor, int a1, int heading, int a3, int a4);
extern int func_8005C7A0(int n);
extern void func_800954E8(unsigned char *actor);

void func_800930E0(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned int substate = B(actor, 0x397);

    if (substate == 0) {
        int dx, dz, dy;
        unsigned int dist, spd, max_spd;
        int offset;
        unsigned int angle;
        int heading, rot;
        int stat_val;

        if (B(actor, 0x396) == 0x8C) {
            func_8005C2AC(actor, 0x61, 9, B(actor, 0x3AB), B(actor, 0x3AC));
            func_8005BA30(actor, 0x61, 0);
            S(actor, 0x3CC) = 0x17;
        } else {
            func_8005C2AC(actor, 0x72, 9, B(actor, 0x3AB), B(actor, 0x3AC));
            func_8005BA30(actor, 0x72, 0);
            S(actor, 0x3CC) = 0x16;
        }

        dx = S(actor, 0x3C2) - S(scratch, 0xF0);
        dz = S(actor, 0x3C6) - S(scratch, 0xF4);
        dist = func_800B1128(dx * dx + dz * dz);
        spd = dist / 10U;

        offset = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        max_spd = (unsigned int)(((B(D_8010C32F, offset) & 0xF) >> 1) + 0x32);
        if (spd > max_spd) {
            spd = max_spd;
        }

        angle = (unsigned int)(func_8005B36C(S(scratch, 0xF0), S(scratch, 0xF4), S(actor, 0x3C2), S(actor, 0x3C6)) & 0xFF);
        SW(actor, 8) = func_8001E694((int)angle, (int)spd);

        dy = S(actor, 0x3C4) - S(scratch, 0xF2);
        SW(actor, 12) = dy / 5;

        offset = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        stat_val = B(D_8010C32F, offset) & 0xF;
        func_80094474((signed int *)(actor + 0xC), 0xFF, (signed short)(-4 - stat_val));

        SW(actor, 16) = func_8001E5A4((int)angle, (int)spd);
        S(actor, 2) = (signed short)(S(actor, 2) + SW(actor, 8));
        S(actor, 6) = (signed short)(S(actor, 6) + SW(actor, 16));

        heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0x40);
        B(actor, 0x3AA) = (unsigned char)heading;
        rot = (0x1C0 - (heading & 0xFF)) % 256;
        S(actor, 0x26) = (signed short)(rot << 4);
        B(actor, 0x397)++;
    } else if (substate == 1) {
        unsigned int frame_idx;

        func_8005BD1C(actor);
        frame_idx = (unsigned int)(B(actor, 0x390) - 1);
        if (frame_idx < 0xFU) {
            switch (frame_idx) {
            case 4:
            case 5:
            case 6:
            case 7:
                S(actor, 4) = (signed short)(S(actor, 4) + SW(actor, 12));
                /* fall through */
            case 0:
            case 1:
            case 2:
            case 3:
            {
                int heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
                int rot;

                B(actor, 0x3AA) = (unsigned char)heading;
                rot = (0x1C0 - (heading & 0xFF)) % 256;
                S(actor, 0x26) = (signed short)(rot << 4);
                S(actor, 2) = (signed short)(S(actor, 2) + SW(actor, 8));
                S(actor, 6) = (signed short)(S(actor, 6) + SW(actor, 16));
                break;
            }
            case 8:
            {
                int heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
                int rot;

                B(actor, 0x3AA) = (unsigned char)heading;
                rot = (0x1C0 - (heading & 0xFF)) % 256;
                S(actor, 0x26) = (signed short)(rot << 4);
                S(actor, 2) = (signed short)(S(actor, 2) + SW(actor, 8));
                S(actor, 4) = (signed short)(S(actor, 4) + SW(actor, 12));
                S(actor, 6) = (signed short)(S(actor, 6) + SW(actor, 16));

                if (func_80094EEC(actor, 0x100) != 0) {
                    S(scratch, 0x31C) += 6;
                    if (func_80094DC4(actor) != 0) {
                        int rnd = func_8001E6B8(8);
                        unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
                        func_800951C4(actor, (B(ball, 0x3A4) + rnd) & 0xFF, 0xE, 0);
                        func_80095090(actor);
                        func_80057968(actor);
                    } else {
                        func_80094FF4(actor);
                        func_8005778C(actor);
                    }
                } else {
                    unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
                    func_8009462C(actor, 0xA, (B(ball, 0x3A4) + 0x80) & 0xFF, 0, 1);
                }
                break;
            }
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
                S(actor, 2) = (signed short)(S(actor, 2) + SW(actor, 8));
                S(actor, 4) = (signed short)(S(actor, 4) - SW(actor, 12));
                S(actor, 6) = (signed short)(S(actor, 6) + SW(actor, 16));
                break;
            case 14:
                S(actor, 4) = 0;
                break;
            }
        }
    }

    if (B(actor, 0x390) == S(actor, 0x3CC)) {
        B(actor, 0x396) = (B(actor, 0x38D) == B(scratch, 0x2A8)) ? 0x81 : 0x82;
        B(actor, 0x397) = 0;
    }

    {
        int jitter = (signed short)func_8005C7A0(4);
        unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
        int ball_vy_scaled = SW(ball, 0xC) / 736;

        if (ball_vy_scaled < 0) {
            ball_vy_scaled = -ball_vy_scaled;
        }
        func_8009462C(actor, 0xA, (B(ball, 0x3A4) + 0x80) & 0xFF, 0, jitter - ball_vy_scaled);
    }
    func_800954E8(actor);
}
