/*
 * func_8002B5A4 - Shot / attacking-zone event & AI reaction handler
 *
 * Purpose:
 *   Triggered on a kick/shot by an actor. If the ball is beyond 17.452 m into
 *   the attacking half (|x| >= 4363 raw), increments team/player shot counters,
 *   records the shooter in D_800FF748+0xAC4E/4F, triggers nearby attackers and
 *   defenders via func_800676C4 / func_800512B8 (setting state 0x1B), and clears
 *   any opposing/teammate players standing in state 7 inside the goal mouth
 *   (|x| > 47.132 m, |z| < 4.608 m) to state 0x1A. Otherwise calls func_80049A94.
 *   Finally checks distance to goal center (> 26.624 m -> calls func_80089D00)
 *   and sets scratch+0x2B0 = 4 (shot event) and scratch+0x31C = attack_dir ^ 1.
 *
 * Arguments:
 *   actor (unsigned char * in $a0) - shooting/kicking player actor pointer.
 *   arg1  (unsigned char in $a1)   - shot type/flags stored to D_800FF748+0xAC4F.
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Actor fields read & written:
 *   Actor +0x02/0x06  (s16) - player x/z
 *   Actor +0x38D      (u8)  - actor id
 *   Actor +0x392      (u8)  - team slot
 *   Actor +0x398      (u8)  - side (0/1)
 *   Actor +0x399      (u8)  - attack direction flag (0/1)
 *   Scratch 0xF2      (s16) - target y (-0x240 = -2.304 m when match_phase == 4)
 *   Scratch 0xF8/0xFC (s16) - target x/z (goal line +/-0x28F0 = +/-41.92 m)
 *   Scratch 0x29A     (u8)  - match phase (4 = set piece)
 *   Scratch 0x2B0     (u32) - ball event code (set to 4)
 *   Scratch 0x2B4/2B6 (s16) - target/landing x/z
 *   Scratch 0x2F4     (ptr) - ball actor pointer
 *   Scratch 0x30C[2]  (s16) - per-direction x reference
 *   Scratch 0x31C     (s16) - set to attack_dir ^ 1
 *   Scratch 0x31E..323      - nearest player IDs per direction
 *   D_800FF748 +0x8EC8[2], +0x88D1[2][22] - team/player shot counters
 *   D_800FF748 +0xAC4E/AC4F               - last shooter id / arg1
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x110B (4363)   = 17.452 m (halfway into attacking half)
 *   0x28F0 (10480)  = 41.92 m (goal-line target x)
 *   -0x240 (-576)   = -2.304 m (crossbar height)
 *   0x2E07 (11783)  = 47.132 m (goal-mouth x threshold)
 *   0x480 (1152)    = 4.608 m (goal-mouth half-width z)
 *   0x260 (608)     = 2.432 m (+/- 0.512 m clear target z)
 *   0x3320 (13088)  = 52.352 m (goal center x)
 *   0x2A40000       = 6656^2 = (26.624 m)^2 distance-squared threshold from goal
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char D_800FF748[];

extern void func_800489A8(void);
extern void func_800676C4(unsigned int dir, int x, int z, unsigned int flag);
extern unsigned int func_80052C6C(unsigned char *actor);
extern unsigned int func_80052D90(unsigned char *actor);
extern void func_800512B8(unsigned char *actor, unsigned int flag);
extern unsigned int func_8005CE30(unsigned int actor_id);
extern int func_8005C7A0(unsigned int range);
extern void func_80049A94(unsigned char *actor, unsigned int a1, unsigned int a2, int x, int z);
extern void func_80089D00(unsigned char *actor, unsigned int a1, int x, int z);

void func_8002B5A4(unsigned char *actor, unsigned char arg1)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
    int ball_x = S(ball, 2);
    int in_zone;
    int px;
    int gx;
    int pz;

    if (B(actor, 0x399) != 0) {
        in_zone = (-ball_x >= 0x110B);
    } else {
        in_zone = (ball_x >= 0x110B);
    }

    if (in_zone) {
        unsigned char *cand;
        unsigned int i;
        unsigned int dir_idx;
        unsigned int opp_gk;
        unsigned char *p;

        B(D_800FF748, 0x8EC8 + B(actor, 0x398)) += 1;
        B(D_800FF748, 0x88D1 + B(actor, 0x398) * 22 + B(actor, 0x392)) += 1;
        B(D_800FF748, 0xAC4F) = arg1;
        B(D_800FF748, 0xAC4E) = B(actor, 0x38D);

        if (B(scratch, 0x29A) == 4) {
            S(scratch, 0xF2) = -0x240;
            func_800489A8();
            func_800676C4(B(actor, 0x399) ^ 1U, S(scratch, 0xF8), S(scratch, 0xFC), 0);
        } else {
            S(scratch, 0xF8) = (B(actor, 0x399) != 0) ? -0x28F0 : 0x28F0;
            S(scratch, 0xFC) = 0;
        }

        func_800676C4(B(actor, 0x399), S(scratch, 0xF8), S(scratch, 0xFC), 0);

        dir_idx = (unsigned int)B(actor, 0x399) * 2U;
        B(scratch, 0x31E + dir_idx) = B(scratch, 0x322 + dir_idx);
        B(scratch, 0x31F + dir_idx) = B(scratch, 0x323 + dir_idx);

        for (i = 0; i < 2; i++) {
            unsigned int idx = B(scratch, 0x322 + (unsigned int)B(actor, 0x399) * 2U + i) % 24U;
            cand = D_800FF748 + idx * 1000;
            if (actor != cand && (func_80052C6C(cand) & 0xFF) == 0) {
                func_800512B8(cand, 1);
                if (B(scratch, 0x29A) != 4) {
                    B(cand, 0x396) = 0x1B;
                    B(cand, 0x397) = 0x30;
                }
                break;
            }
        }

        func_800676C4(B(actor, 0x399) ^ 1U, S(scratch, 0xF8), S(scratch, 0xFC), 0);

        dir_idx = (unsigned int)(B(actor, 0x399) ^ 1U) * 2U;
        B(scratch, 0x31E + dir_idx) = B(scratch, 0x322 + dir_idx);
        B(scratch, 0x31F + dir_idx) = B(scratch, 0x323 + dir_idx);

        for (i = 0; i < 2; i++) {
            unsigned int idx = B(scratch, 0x31E + (unsigned int)(B(actor, 0x399) ^ 1U) * 2U + i) % 24U;
            cand = D_800FF748 + idx * 1000;
            if ((func_80052D90(cand) & 0xFF) == 0) {
                func_800512B8(cand, 1);
                B(cand, 0x396) = 0x1B;
                B(cand, 0x397) = 0;
                if (B(scratch, 0x29A) == 4) {
                    S(cand, 0x3C2) = 0xFF;
                } else {
                    B(cand, 0x397) = 0x30;
                }
                break;
            }
        }

        opp_gk = func_8005CE30(B(actor, 0x38D)) & 0xFF;
        p = D_800FF748 + (opp_gk + 1U) * 1000;
        for (i = 0; i < 10; i++, p += 1000) {
            int near_goal_x;
            int abs_z;

            if (B(p, 0x399) != 0) {
                near_goal_x = (-S(p, 2) < -0x2E07);
            } else {
                near_goal_x = (S(p, 2) < -0x2E07);
            }
            if (!near_goal_x) {
                continue;
            }

            abs_z = S(p, 6);
            if (abs_z < 0) {
                abs_z = -abs_z;
            }
            if (abs_z < 0x480 && B(p, 0x396) == 7) {
                signed short off_z;
                B(p, 0x396) = 0x1A;
                B(p, 0x397) = 0;
                off_z = (signed short)func_8005C7A0(0x80) + 0x260;
                if (S(p, 6) < 0) {
                    off_z = (signed short)(-off_z);
                }
                S(p, 0x3C6) = off_z;
            }
        }
    } else {
        func_80049A94(actor, 2, 0, S(scratch, 0x2B4), S(scratch, 0x2B6));
    }

    px = S(actor, 2);
    pz = S(actor, 6);
    gx = (B(actor, 0x399) != 0) ? (px + 0x3320) : (px - 0x3320);
    if (gx * gx + pz * pz > 0x2A40000) {
        func_80089D00(actor, 0, S(scratch, 0x30C + (unsigned int)B(actor, 0x399) * 2U),
                      S(scratch, 0x2B6));
    }

    W(scratch, 0x2B0) = 4;
    H(scratch, 0x31C) = (unsigned short)(B(actor, 0x399) ^ 1U);
}
