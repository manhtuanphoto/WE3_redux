/*
 * func_8008CDEC - Player intercept / lunge-clear action state handler
 *
 * Purpose:
 *   Handles a player actor's intercept/lunge action across substates (+0x397):
 *   - Substate 0: reaction-delay check against scratch+0x31C; starts anim 0x74,
 *     turns toward +0x3AB, computes horizontal lunge velocity toward (+0x3C2,+0x3C6).
 *   - Substate 1: advances anim and lunge position (damping velocity by 9/10 from
 *     frame 5 onward), then transitions to recovery anim 0x2C frame 0x1A.
 *   - Substate 2: tracks ball heading during open play until frame 0x2E, then
 *     resets to state 0x82 / substate 0 / anim 1.
 *   - Substate 16 (0x10): tests approach angle vs predicted ball trajectory and
 *     latches target (+0x3C2/+0x3C4/+0x3C6) + mirror flag (+0x3AC), resetting substate to 0.
 *   For substates 0, 1, 2 (and default), tests ball/contact via func_8009462C and
 *   calls func_80057A14 on hit, then finishes with func_800954E8.
 *
 * Arguments:
 *   actor (unsigned char * in $a0) - pointer to 1000-byte player actor struct.
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Actor fields read & written:
 *   Actor +0x02/0x06  (s16) - x/z position
 *   Actor +0x08/0x10  (s32) - x/z velocity
 *   Actor +0x26       (s16) - render rotation
 *   Actor +0x38D      (u8)  - actor id
 *   Actor +0x390      (u8)  - anim frame
 *   Actor +0x392      (u8)  - team slot
 *   Actor +0x396      (u8)  - state (set to 0x82 at end of substate 2)
 *   Actor +0x397      (u8)  - substate (0, 1, 2, 16)
 *   Actor +0x398      (u8)  - side (0/1)
 *   Actor +0x3A0      (u32) - speed (cleared to 0 on substate 1->2)
 *   Actor +0x3AA      (u8)  - body heading
 *   Actor +0x3AB      (u8)  - target heading
 *   Actor +0x3AC      (u8)  - mirror/side flag
 *   Actor +0x3B0      (u8)  - cleared to 0 on substate 1->2
 *   Actor +0x3B9      (u8)  - reaction/stamina stat (0..100)
 *   Actor +0x3C2/3C4/3C6 (s16) - target x/y/z
 *   Scratch 0x40/44/4E/58/70/74 (s16) - predicted ball trajectory samples
 *   Scratch 0x29A     (u8)  - match phase (1 = open play)
 *   Scratch 0x2AC     (u8)  - active/exception actor id
 *   Scratch 0x2F4     (ptr) - ball actor pointer
 *   Scratch 0x31C     (s16) - ball timer (frames since last touch)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   64 (0x40)         = 0.256 m minimum distance to lunge
 *   32 (0x20)         = max lunge step speed (0.128 m/frame = 3.84 m/s)
 *   0x58 (88) / 0x30 (48) = offset angles (~123.75 deg / 67.5 deg) by height check
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char D_8010C330[];

extern void func_8006EC5C(unsigned char *actor);
extern void func_8005BA30(unsigned char *actor, unsigned int anim, unsigned int frame);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_800B1128(unsigned int val);
extern int func_8001E694(unsigned int angle, int scale);
extern int func_8001E5A4(unsigned int angle, int scale);
extern void func_8005BD1C(unsigned char *actor);
extern int func_8001E6B8(int n);
extern unsigned int func_8009462C(unsigned char *actor, unsigned int radius, unsigned int angle,
                                  unsigned int flag, unsigned int power);
extern void func_80057A14(unsigned char *actor);
extern void func_800954E8(unsigned char *actor);

void func_8008CDEC(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char substate = B(actor, 0x397);

    switch (substate) {
    case 0: {
        int delay = 14 - (int)(B(actor, 0x3B9) / 10U);
        unsigned int heading;
        int turn;
        unsigned int target_ang;
        int dx;
        int dz;
        unsigned int dist;

        if (S(scratch, 0x31C) < delay && B(actor, 0x38D) != B(scratch, 0x2AC)) {
            func_8006EC5C(actor);
            break;
        }

        func_8005BA30(actor, 0x74, 0);
        heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 12);
        B(actor, 0x3AA) = (unsigned char)heading;
        turn = 0x1C0 - (int)(heading & 0xFF);
        S(actor, 0x26) = (signed short)((turn % 256) << 4);

        target_ang = func_8005B36C(S(actor, 2), S(actor, 6), S(actor, 0x3C2), S(actor, 0x3C6));
        dx = S(actor, 2) - S(actor, 0x3C2);
        dz = S(actor, 6) - S(actor, 0x3C6);
        dist = func_800B1128((unsigned int)(dx * dx + dz * dz));

        if (dist >= 0x40U) {
            unsigned int spd = (dist - 0x40U) / 5U;
            if (spd >= 0x20U) {
                spd = 0x20U;
            }
            SW(actor, 8) = func_8001E694(target_ang & 0xFF, (int)spd);
            SW(actor, 0x10) = func_8001E5A4(target_ang & 0xFF, (int)spd);
            S(actor, 2) = (signed short)(S(actor, 2) + SW(actor, 8));
            S(actor, 6) = (signed short)(S(actor, 6) + SW(actor, 0x10));
        } else {
            SW(actor, 8) = 0;
            SW(actor, 0x10) = 0;
        }
        B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
        break;
    }

    case 1: {
        unsigned int heading;
        int turn;

        func_8005BD1C(actor);
        heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 12);
        B(actor, 0x3AA) = (unsigned char)heading;
        turn = 0x1C0 - (int)(heading & 0xFF);
        S(actor, 0x26) = (signed short)((turn % 256) << 4);

        S(actor, 2) = (signed short)(S(actor, 2) + SW(actor, 8));
        S(actor, 6) = (signed short)(S(actor, 6) + SW(actor, 0x10));

        if (B(actor, 0x390) >= 5) {
            SW(actor, 8) = (SW(actor, 8) * 9) / 10;
            SW(actor, 0x10) = (SW(actor, 0x10) * 9) / 10;
        }
        if (B(actor, 0x390) == 0) {
            func_8005BA30(actor, 0x2C, 0x1A);
            B(actor, 0x3B0) = 0;
            W(actor, 0x3A0) = 0;
            B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
        }
        break;
    }

    case 2:
        func_8005BD1C(actor);
        if (B(scratch, 0x29A) == 1) {
            unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
            unsigned int ang = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));
            unsigned int heading = func_8005B7CC(ang & 0xFF, B(actor, 0x3AA), 12);
            int turn = 0x1C0 - (int)(heading & 0xFF);

            B(actor, 0x3AA) = (unsigned char)heading;
            S(actor, 0x26) = (signed short)((turn % 256) << 4);
        }
        if (B(actor, 0x390) == 0x2E) {
            func_8005BA30(actor, 1, 0);
            B(actor, 0x396) = 0x82;
            B(actor, 0x397) = 0;
        }
        break;

    case 16: {
        unsigned int pred_ang = func_8005B36C(S(actor, 2), S(actor, 6), S(scratch, 0x44), S(scratch, 0x74));
        unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
        unsigned int ball_heading = B(ball, 0x3A4);
        unsigned int diff = (pred_ang - ball_heading) & 0xFF;
        int in_cone;

        if (diff < 129U) {
            in_cone = (diff < 64U);
        } else {
            in_cone = (((ball_heading - pred_ang) & 0xFF) < 64U);
        }

        if (in_cone) {
            unsigned char *b2 = (unsigned char *)W(scratch, 0x2F4);
            unsigned int ang_from_ball = func_8005B36C(S(b2, 2), S(b2, 6), S(actor, 2), S(actor, 6));

            B(actor, 0x3AC) = (unsigned char)(((ang_from_ball - pred_ang) & 0xFF) < 129U);
            H(actor, 0x3C2) = H(scratch, 0x40);
            H(actor, 0x3C4) = H(scratch, 0x58);
            H(actor, 0x3C6) = H(scratch, 0x70);
            B(actor, 0x397) = 0;
        }
        return;
    }
    }

    {
        unsigned int stat_off = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        int height_cm = (int)(((W(D_8010C330, stat_off) >> 11) & 0x7F) + 100);
        int height_thresh = -(height_cm * 160) / 180;
        unsigned int offset_ang = (S(scratch, 0x4E) < height_thresh) ? 0x30U : 0x58U;
        unsigned int kick_ang;
        unsigned int power;

        if (B(actor, 0x3AC) != 0) {
            kick_ang = (B(actor, 0x3AA) + 0x80U) - offset_ang;
        } else {
            kick_ang = (B(actor, 0x3AA) + 0x80U) + offset_ang;
        }
        power = (unsigned int)(func_8001E6B8(8) + 16);

        if ((func_8009462C(actor, 12, kick_ang & 0xFF, 0, power) & 0xFF) != 0) {
            func_80057A14(actor);
        }
        func_800954E8(actor);
    }
}
