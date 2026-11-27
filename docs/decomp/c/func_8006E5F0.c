/*
 * func_8006E5F0
 *
 * Purpose:
 *   AI / player off-ball ball-chase / pressing state (state 0x82 handler or
 *   ball-pursuit substate). Initializes anim 0x26 on substate 0, computes a
 *   predicted interception target point in Scratchpad+0xF8/+0xFC when
 *   actor+0x3B2 == 1, checks for air-ball / loose-ball / close-ball triggers
 *   (states 0x24, 0x0D, 0x0A), and otherwise steers toward the ball/target or
 *   claims/tracks a free ball. Exits back to state 0x82 if ball/team flags no
 *   longer hold or match phase != 1.
 *
 * Arguments:
 *   actor - pointer to 1000-byte player actor struct
 *
 * Return value:
 *   None (void)
 *
 * Actor / global table fields read:
 *   actor+0x02 (s16) x, actor+0x06 (s16) z,
 *   actor+0x38D (u8) actor id, actor+0x390 (u8) anim frame,
 *   actor+0x397 (u8) substate, actor+0x398 (u8) side,
 *   actor+0x399 (u8) attack direction flag, actor+0x3A0 (u32) speed,
 *   actor+0x3AA (u8) body heading, actor+0x3B2 (u8) intercept/target mode flag,
 *   actor+0x3BE (s16) timer/lockout, actor+0x3D4 (u16) active bitmask,
 *   D_800FF748[targetIdx * 1000] target actor (+0x02, +0x06, +0x08, +0x10)
 *
 * Actor fields written:
 *   actor+0x04 (s16) y height (cleared to 0 in substate 0),
 *   actor+0x396 (u8) state (0x24, 0x0D, 0x0A, or 0x82),
 *   actor+0x397 (u8) substate,
 *   actor+0x3A0 (u32) speed (decelerated by 18 when within 64 units of free ball),
 *   actor+0x3A7 (u8) set to 1, actor+0x3AA (u8) body heading,
 *   actor+0x3AE (u16) decremented by 2, actor+0x3D1 (u8) set to 1
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x032[side] (u16) active bitmask,
 *   +0x036 (s16) ball predicted x frame 0, +0x04E (s16) ball predicted y frame 0,
 *   +0x05E (s16) ball predicted y frame 8, +0x066 (s16) ball predicted z frame 0,
 *   +0x0F8 / +0x0FC (s16) predicted target x / z,
 *   +0x29A (u8) match phase (1 = open play), +0x2A9 (u8) ball owner,
 *   +0x2AC (u8) designated presser/chaser actor id,
 *   +0x2CF[side] (u8) possession flag, +0x2F4 (u8*) ball actor pointer,
 *   +0x31E[side] (u8) target actor index
 *
 * Scratchpad / ball fields written:
 *   +0x0F8 / +0x0FC (s16) predicted target x / z (clamped to +-0x3300),
 *   ball+0x3DC (u16) set to 2 when within 64 units of free ball
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x33A0 = 13216 (52.864 m goal-line X), 0x3300 = 13056 (52.224 m X clamp),
 *   -0x160 = -352 (-1.408 m height threshold at frame 8),
 *   -0x7F = -127 (-0.508 m height threshold at frame 0),
 *   0x3FFFFF = dist^2 (2048 raw = 8.192 m), 0x8FFFF = dist^2 (768 raw = 3.072 m),
 *   0x23FFF = dist^2 (384 raw = 1.536 m), 0x40000 = dist^2 (512 raw = 2.048 m),
 *   0x40 = 64 (0.256 m), 0x100 = 256 (1.024 m), 0x300 = 768 (3.072 m),
 *   0x28 = 40 deg-byte (~56.25 deg), 0x30 = 48 (~67.5 deg), 0x60 = 96 (~135 deg)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];

extern int func_8001E5A4(unsigned int angle, int scale);
extern int func_8001E694(unsigned int angle, int scale);
extern void func_800479B8(unsigned char *actor, int mode);
extern void func_80048AD8(unsigned char *actor, unsigned int angle1, unsigned int angle2, unsigned int dist);
extern unsigned int func_800494C0(unsigned char *actor);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, int step);
extern void func_8005B908(unsigned char *actor);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005BA78(unsigned char *actor, int anim, int frame);
extern void func_8005BD1C(unsigned char *actor);
extern unsigned int func_800B1128(unsigned int val);

void func_8006E5F0(unsigned char *actor)
{
    unsigned char substate;
    unsigned int predDistSq;
    unsigned int pressure;
    unsigned char *ball;

    H(actor, 0x3AE) = H(actor, 0x3AE) - 2;
    B(actor, 0x3A7) = 1;
    B(actor, 0x3D1) = 1;

    substate = B(actor, 0x397);
    if (substate == 0) {
        func_8005BA78(actor, 0x26, B(actor, 0x390));
        S(actor, 0x04) = 0;
        B(actor, 0x397) = B(actor, 0x397) + 1;
    } else if (substate != 1) {
        goto check_exit;
    }

    func_8005BD1C(actor);

    if (B(actor, 0x3B2) == 1) {
        unsigned int atkDir = B(actor, 0x399);
        unsigned int targetIdx = B(Scratchpad, 0x31E + ((atkDir ^ 1) << 1));
        unsigned char *targetActor = D_800FF748 + (targetIdx % 24) * 1000;
        int goalX = (atkDir != 0) ? 0x33A0 : -0x33A0;
        unsigned int goalAngle;
        unsigned int actorAngle;
        unsigned int diff;
        int dx;
        int dz;
        unsigned int dist;
        int offsetDist;
        unsigned int goalAngleByte;

        goalAngle = func_8005B36C(S(targetActor, 0x02), S(targetActor, 0x06), goalX, 0);
        actorAngle = func_8005B36C(S(targetActor, 0x02), S(targetActor, 0x06), S(actor, 0x02), S(actor, 0x06));

        if (((actorAngle - goalAngle) & 0xFF) < 0x81) {
            actorAngle = func_8005B36C(S(targetActor, 0x02), S(targetActor, 0x06), S(actor, 0x02), S(actor, 0x06));
            diff = actorAngle - goalAngle;
        } else {
            actorAngle = func_8005B36C(S(targetActor, 0x02), S(targetActor, 0x06), S(actor, 0x02), S(actor, 0x06));
            diff = goalAngle - actorAngle;
        }

        dx = (S(actor, 0x02) - S(targetActor, 0x02)) + (SW(targetActor, 0x08) << 2);
        dz = (S(actor, 0x06) - S(targetActor, 0x06)) + (SW(targetActor, 0x10) << 2);
        dist = func_800B1128((unsigned int)(dx * dx + dz * dz));

        diff &= 0xFF;
        if (diff >= 5) {
            diff = 5;
        }
        offsetDist = ((int)dist * (int)(diff + 4)) / 10;

        goalAngleByte = goalAngle & 0xFF;
        S(Scratchpad, 0x0F8) = S(targetActor, 0x02) + (SW(targetActor, 0x08) << 2)
                             + func_8001E694(goalAngleByte, offsetDist);
        S(Scratchpad, 0x0FC) = S(targetActor, 0x06) + (SW(targetActor, 0x10) << 2)
                             + func_8001E5A4(goalAngleByte, offsetDist);
    }

    if (S(Scratchpad, 0x0F8) >= 0x3300) {
        S(Scratchpad, 0x0F8) = 0x3300;
    } else if (S(Scratchpad, 0x0F8) < -0x32FF) {
        S(Scratchpad, 0x0F8) = -0x3300;
    }

    {
        int dx = S(actor, 0x02) - S(Scratchpad, 0x0F8);
        int dz = S(actor, 0x06) - S(Scratchpad, 0x0FC);
        predDistSq = (unsigned int)(dx * dx + dz * dz);
    }
    pressure = func_800494C0(actor);

    if (S(actor, 0x3BE) == 0) {
        if (S(Scratchpad, 0x05E) < -0x160) {
            int bx;
            int bz;
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            bx = S(actor, 0x02) - S(ball, 0x02);
            bz = S(actor, 0x06) - S(ball, 0x06);
            if (bx * bx + bz * bz <= 0x3FFFFF) {
                B(actor, 0x396) = 0x24;
                B(actor, 0x397) = 0;
                goto check_exit;
            }
        }

        if (B(Scratchpad, 0x2A9) != 0 && B(Scratchpad, 0x2CF + B(actor, 0x398)) == 1 && predDistSq <= 0x8FFFF) {
            unsigned int angleToBall;
            unsigned int bodyHeading;
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            angleToBall = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(ball, 0x02), S(ball, 0x06));
            bodyHeading = B(actor, 0x3AA);

            if (((bodyHeading - angleToBall) & 0xFF) < 0x81) {
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                angleToBall = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(ball, 0x02), S(ball, 0x06));
                bodyHeading = B(actor, 0x3AA);
                if (((bodyHeading - angleToBall) & 0xFF) < 0x28) {
                    B(actor, 0x396) = 0x0D;
                    B(actor, 0x397) = 0;
                    goto check_exit;
                }
            } else {
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                angleToBall = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(ball, 0x02), S(ball, 0x06));
                bodyHeading = B(actor, 0x3AA);
                if (((angleToBall - bodyHeading) & 0xFF) < 0x28) {
                    B(actor, 0x396) = 0x0D;
                    B(actor, 0x397) = 0;
                    goto check_exit;
                }
            }
        }

        if (predDistSq <= 0x23FFF && S(Scratchpad, 0x04E) >= -0x7F) {
            int vy;
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            vy = SW(ball, 0x0C);
            if (vy < 0) {
                vy = -vy;
            }
            if (vy < 0x1700 && pressure > 0x40000) {
                B(actor, 0x396) = 0x0A;
                B(actor, 0x397) = 0;
                goto check_exit;
            }
        }
    }

    if (B(Scratchpad, 0x2A9) != 0) {
        if (B(Scratchpad, 0x2AC) == B(actor, 0x38D) && (predDistSq + 0x40000) < pressure) {
            func_800479B8(actor, 0);
        } else {
            func_800479B8(actor, 1);
        }
    } else {
        unsigned int angleToBallCur = func_8005B36C(
            S(actor, 0x02), S(actor, 0x06),
            S(Scratchpad, 0x036), S(Scratchpad, 0x066));
        unsigned int predDist = func_800B1128(predDistSq);

        if (predDist < 0x40) {
            unsigned int spd;
            func_8005BA30(actor, 0x0E, 0x0F);
            B(actor, 0x3AA) = (unsigned char)func_8005B7CC(angleToBallCur & 0xFF, B(actor, 0x3AA), 0x10);
            spd = W(actor, 0x3A0);
            W(actor, 0x3A0) = (spd < 0x13) ? 0 : (spd - 0x12);
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            H(ball, 0x3DC) = 2;
        } else {
            unsigned int angleToTarget = func_8005B36C(
                S(actor, 0x02), S(actor, 0x06),
                S(Scratchpad, 0x0F8), S(Scratchpad, 0x0FC));
            unsigned int angleDiff = angleToTarget - angleToBallCur;

            if ((angleDiff & 0xFF) >= 0x81) {
                angleDiff = angleToBallCur - angleToTarget;
            }
            angleDiff &= 0xFF;

            if (predDist < 0x300 && angleDiff < 0x60 && (predDist >= 0x100 || angleDiff < 0x30)) {
                func_800479B8(actor, 1);
            } else {
                func_80048AD8(actor, angleToTarget & 0xFF, angleToBallCur & 0xFF, predDist & 0xFFFF);
            }
        }
    }

check_exit:
    if ((H(Scratchpad, 0x032 + (B(actor, 0x398) << 1)) & H(actor, 0x3D4)) == 0
        || B(Scratchpad, 0x29A) != 1) {
        B(actor, 0x396) = 0x82;
        B(actor, 0x397) = 0;
    }

    func_8005B908(actor);
}
