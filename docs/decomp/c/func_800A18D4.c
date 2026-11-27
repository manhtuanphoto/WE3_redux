/*
 * func_800A18D4 - Select an attacking teammate to make a support/through run
 *
 * Purpose:
 *   During open play (Scratchpad+0x29A == 1), evaluates the team referenced by
 *   D_801025D8 / Scratchpad+0x2CF. Computes a helper flag at Scratchpad+0x05 via
 *   func_800658D0, checks whether the ball is in the attacking zone and within
 *   Z bounds, counts how many outfielders already have role-run state +0x3E2 == 16
 *   (up to the limit configured by Scratchpad+0x2FF[side]), and picks the closest
 *   eligible teammate (either within a heading cone of the ball owner when
 *   |ball.x| < 9186, or deep inside the attacking box) to receive +0x3E2 = 16
 *   and +0x3E3 = 0.
 *
 * Arguments:
 *   None (void).
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Actor fields read & written:
 *   Scratch 0x05       (u8)  - written with result of func_800658D0(..., 2)
 *   Scratch 0x06..0x08 (u8)  - excluded actor IDs
 *   Scratch 0x29A      (u8)  - match phase (must be 1 = open play)
 *   Scratch 0x2A9      (u8)  - ball owner actor ID (must be != 0)
 *   Scratch 0x2CF[2]   (u8)  - possession flags per side
 *   Scratch 0x2F4      (ptr) - ball actor pointer (+0x02 x, +0x06 z)
 *   Scratch 0x2FF[2]   (u8)  - team run-support level / max count (0, 1, or >=2)
 *   Scratch 0x31E[4]   (u8)  - tracked player IDs indexed by attack_dir * 2
 *   Actor +0x00        (u8)  - active flag (must be != 0)
 *   Actor +0x02/+0x06  (s16) - x / z position
 *   Actor +0x38D       (u8)  - actor ID
 *   Actor +0x392       (u8)  - team slot (0..10)
 *   Actor +0x398       (u8)  - team side (0/1)
 *   Actor +0x399       (u8)  - attack direction (0 or 1)
 *   Actor +0x39A       (u8)  - owner bit 0 used for cone width (+12 if set)
 *   Actor +0x3AA       (u8)  - owner body heading
 *   Actor +0x3DA       (u8)  - cleared to 0 on best candidate selection
 *   Actor +0x3E2       (u8)  - run state (16 = active support run, written)
 *   Actor +0x3E3       (u8)  - run sub-timer/flag (cleared to 0, written)
 *   D_801025D8         (u8)  - side index (also read via D_800FF748+0x2E90)
 *   D_8010C1DC         (u8)  - selected candidate actor ID
 *   D_8010C32E[]       (u16) - player stat table (stride 264/team, 12/slot)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0xBC1  (3009 = 12.036 m)  = minimum attacking X when attack_dir == 0
 *   -0xBC0 (-3008 = -12.032 m) = maximum attacking X threshold when attack_dir == 1
 *   0x12F4 (4852 = 19.408 m)  = max |Z| lateral bound
 *   0x23E2 (9186 = 36.744 m)  = |ball.x| threshold switching between cone search and box search
 *   0x1BE3 (7139 = 28.556 m)  = minimum attacking depth (actor.x * dir) for box search
 *   0x3100000 (51380224, ~7168^2 = 28.672^2 m^2) = initial best squared distance sentinel
 *   35 (+12 if bit0, -stat&0xF, +level*5) = heading cone half-angle in byte units (~49 deg base)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_801025D8;
extern unsigned char D_8010C1DC;
extern unsigned char D_8010C32E[];

extern unsigned char func_800658D0(unsigned char *actor, int mode);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);

void func_800A18D4(void)
{
    unsigned char phase = B(Scratchpad, 0x29A);
    unsigned char *team_end;
    unsigned char *ball;
    unsigned char *owner;
    unsigned char *actor;
    unsigned char atk_dir;
    unsigned char count;
    unsigned char owner_id;
    unsigned char in_mid_zone;
    unsigned char level;
    int abs_bz;
    int abs_bx;
    int dir;
    int best_dist_sq;
    int i;

    if (phase != 1) {
        return;
    }

    if (B(Scratchpad, 0x2CF + D_801025D8) == 0) {
        team_end = D_800FF748 + 11 * 1000;
    } else {
        team_end = D_800FF748 + 22 * 1000;
    }

    atk_dir = B(team_end, 0x399);
    {
        unsigned int ref_id = B(Scratchpad, 0x31E + atk_dir * 2);
        B(Scratchpad, 0x05) = func_800658D0(D_800FF748 + (ref_id % 24) * 1000, 2);
    }

    atk_dir = B(team_end, 0x399);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    if (atk_dir == 0) {
        if (S(ball, 2) < 0xBC1) {
            return;
        }
    } else {
        if (atk_dir != 1 || S(ball, 2) >= -0xBC0) {
            return;
        }
    }

    abs_bz = S(ball, 6);
    if (abs_bz < 0) {
        abs_bz = -abs_bz;
    }
    if (abs_bz >= 0x12F4) {
        return;
    }

    count = 0;
    actor = team_end;
    for (i = 0; i < 10; i++, actor -= 1000) {
        if (B(actor, 0x3E2) == 0x10) {
            count++;
        }
    }

    owner_id = B(Scratchpad, 0x2A9);
    if (owner_id == 0) {
        return;
    }

    best_dist_sq = 0x3100000;
    owner = D_800FF748 + ((unsigned int)owner_id % 24) * 1000;
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    abs_bx = S(ball, 2);
    if (abs_bx < 0) {
        abs_bx = -abs_bx;
    }
    in_mid_zone = (abs_bx < 0x23E2);

    if (B(Scratchpad, 0x2CF + B(D_800FF748, 0x2E90)) != 0) {
        team_end = D_800FF748 + 22 * 1000;
    } else {
        team_end = D_800FF748 + 11 * 1000;
    }

    level = B(Scratchpad, 0x2FF + B(team_end, 0x398));
    if (level == 1) {
        if (count == 1) {
            return;
        }
    } else if (level == 0) {
        if (count == 0) {
            return;
        }
    } else {
        if (count == 2) {
            return;
        }
    }

    dir = (B(team_end, 0x399) != 0) ? -1 : 1;

    actor = team_end;
    for (i = 0; i < 10; i++, actor -= 1000) {
        unsigned char cand_id = B(actor, 0x38D);
        unsigned char cand_atk = B(actor, 0x399);

        if (cand_id == B(Scratchpad, 0x2A9)) {
            continue;
        }
        if (B(actor, 0x00) == 0) {
            continue;
        }
        if (cand_id == B(Scratchpad, 0x31E + cand_atk * 2)) {
            continue;
        }
        if (cand_id == B(Scratchpad, 0x06) ||
            cand_id == B(Scratchpad, 0x07) ||
            cand_id == B(Scratchpad, 0x08)) {
            continue;
        }

        if (in_mid_zone == 1) {
            unsigned char cand_side = B(actor, 0x398);
            unsigned char cand_slot = B(actor, 0x392);
            int cone = (B(owner, 0x39A) & 1) * 12 + 35;
            unsigned short stat_val = H(D_8010C32E + cand_slot * 12 + cand_side * 264);
            int max_angle;
            unsigned int ang;
            unsigned int diff;
            int dx, dz, dist_sq;

            cone -= (stat_val & 0xF);
            max_angle = cone + B(Scratchpad, 0x2FF + cand_side) * 5;

            ang = func_8005B36C(S(owner, 2), S(owner, 6), S(actor, 2), S(actor, 6));
            if (((ang - B(owner, 0x3AA)) & 0xFF) < 0x81) {
                ang = func_8005B36C(S(owner, 2), S(owner, 6), S(actor, 2), S(actor, 6));
                diff = (ang - B(owner, 0x3AA)) & 0xFF;
            } else {
                ang = func_8005B36C(S(owner, 2), S(owner, 6), S(actor, 2), S(actor, 6));
                diff = (B(owner, 0x3AA) - ang) & 0xFF;
            }

            if ((int)diff < max_angle) {
                dx = S(actor, 2) - S(owner, 2);
                dz = S(actor, 6) - S(owner, 6);
                dist_sq = dx * dx + dz * dz;
                if (dist_sq < best_dist_sq) {
                    D_8010C1DC = cand_id;
                    best_dist_sq = dist_sq;
                    B(actor, 0x3DA) = 0;
                }
            }
        } else {
            int cx = S(actor, 2);
            int cz = S(actor, 6);
            int abs_cz = (cz >= 0) ? cz : -cz;
            int dx, dz, dist_sq;

            if (cx * dir < 0x1BE3) {
                continue;
            }
            if (abs_cz >= 0x12F4) {
                continue;
            }
            dx = cx - S(owner, 2);
            dz = cz - S(owner, 6);
            dist_sq = dx * dx + dz * dz;
            if (dist_sq < best_dist_sq) {
                best_dist_sq = dist_sq;
                D_8010C1DC = cand_id;
                B(actor, 0x3DA) = 0;
            }
        }
    }

    {
        unsigned int chosen_id = D_8010C1DC;
        unsigned char *chosen = D_800FF748 + (chosen_id % 24) * 1000;
        B(chosen, 0x3E2) = 0x10;
        B(chosen, 0x3E3) = 0;
    }
}
