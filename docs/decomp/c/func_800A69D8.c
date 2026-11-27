/*
 * func_800A69D8
 *
 * Purpose:
 *   Set-piece / free-kick player positioning and defensive wall builder.
 *   Resets all 22 players to their formation/set-piece targets (GKs at +-0x3120,
 *   outfielders via func_800A5980), picks the kicker at SCRATCH+0x2AD if unassigned,
 *   computes defensive wall count and adjusts it if 4+ teammates are active,
 *   positions the defending GK on the goal line aligned to block the near/far post,
 *   and places the wall players 0x928 raw (9.376 m) from the ball spaced 185 raw (0.74 m) apart.
 *
 * Arguments:
 *   void
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Actor fields (all 22 players in D_800FF748):
 *     +0x000 (u8)  active flag (R)
 *     +0x002 (s16) x (W)
 *     +0x006 (s16) z (W)
 *     +0x38D (u8)  actor id (R)
 *     +0x396 (u8)  state (W: 0x80 for GK, 0x35 for wall, 0 for others)
 *     +0x397 (u8)  substate (W: 0)
 *     +0x399 (u8)  attack direction flag (R)
 *     +0x39A (u8)  skip flag when SCRATCH+0x2E1 == 5 (R)
 *   Ball actor (SCRATCH+0x2F4):
 *     +0x002 (s16) ball x (R)
 *     +0x006 (s16) ball z (R)
 *   Scratchpad (0x1F800000):
 *     +0x2AD (u8)  set-piece kicker actor id (R/W)
 *     +0x2AE (u8)  fouled/set-piece actor index (R)
 *     +0x2E1 (u8)  mode (R: 5 flips side flag)
 *     +0x2F4 (u8*) ptr to ball actor (R)
 *   Globals:
 *     D_800FF748 (u8[]) actor array (1000-byte stride)
 *     D_800FFAE1 (u8[]) offset 0x399 inside D_800FF748 (actor attack_dir)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x3120 = 12576 raw (50.304 m) - GK reset X
 *   0x3220 = 12832 raw (51.328 m) - GK wall-setup X
 *   0x3320 = 13088 raw (52.352 m) - goal line X
 *   0x23E3 = 9187 raw (36.748 m)  - attacking third X
 *   0x12F4 = 4852 raw (19.408 m), 0x12F5 = 4853 raw - penalty box Z half-width
 *   0xE01  = 3585 raw (14.34 m)   - long-range FK threshold X
 *   0x928  = 2344 raw (9.376 m)   - wall distance from ball (~10 yards)
 *   185 raw (0.74 m)              - spacing between adjacent wall players
 *   0x393  = 915 raw (3.66 m)     - goal half-width Z threshold for wall aim offset
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_800FFAE1[];

extern void func_800A484C(void);
extern void func_8009F554(unsigned char *, int, int);
extern void func_800A5980(unsigned char *, short *, short *, unsigned int);
extern void func_800A3730(void);
extern void func_800A3590(unsigned char *);
extern void func_800688B8(unsigned char *, unsigned int);
extern unsigned int func_80068E24(unsigned int);
extern unsigned char func_8007BA04(unsigned char *);
extern unsigned char func_8005CE14(unsigned int);
extern void func_80068A38(unsigned char *, unsigned int);
extern unsigned int func_8005B36C(int, int, int, int);
extern int func_8001E694(unsigned int, int);
extern int func_8001E5A4(unsigned int, int);
extern void func_8009F574(void);

void func_800A69D8(void)
{
    unsigned char *ball;
    unsigned short ball_x;
    unsigned short ball_z;
    unsigned int side_flag;
    unsigned int team_loop;
    unsigned int slot_loop;
    unsigned char *actor;
    short tx;
    short tz;
    unsigned char wall_ids[16];
    unsigned int wall_count;
    int abs_val;
    unsigned int active_cnt;
    unsigned int idx;
    int abs_bz;
    unsigned char *kicker;
    unsigned int gk_ang;
    unsigned int gk_diff;
    int goal_x;
    int goal_z;
    unsigned int wall_ang;
    unsigned int perp_raw;
    int perp_mod;
    unsigned int perp_ang;
    unsigned char *first_wall;
    unsigned int i;
    int spacing;

    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    ball_x = H(ball, 2);
    ball_z = H(ball, 6);

    func_800A484C();

    side_flag = B(D_800FFAE1, B(SCRATCH, 0x2AE) * 1000);

    for (team_loop = 0; (team_loop & 0xFFU) < 2U; team_loop++) {
        if ((team_loop & 0xFFU) != 0) {
            actor = D_800FF748 + 0x2EE0;
        } else {
            actor = D_800FF748 + 0x3E8;
        }

        for (slot_loop = 0; (slot_loop & 0xFFU) < 11U; slot_loop++, actor += 0x3E8) {
            if (B(SCRATCH, 0x2E1) == 5 && B(actor, 0x39A) != 0) {
                continue;
            }
            if (B(actor, 0x38D) == 1) {
                func_8009F554(actor, -0x3120, 0);
                B(actor, 0x396) = 0x80;
            } else if (B(actor, 0x38D) == 0xC) {
                func_8009F554(actor, 0x3120, 0);
                B(actor, 0x396) = 0x80;
            } else {
                func_800A5980(actor, &tx, &tz, side_flag & 0xFFU);
                func_8009F554(actor, tx, tz);
                B(actor, 0x396) = 0;
            }
            B(actor, 0x397) = 0;
        }
    }

    func_800A3730();
    func_800A3590(wall_ids);
    func_800688B8(wall_ids, side_flag & 0xFFU);
    wall_count = func_80068E24(side_flag & 0xFFU);

    if ((wall_count & 0xFFU) != 0) {
        abs_val = S((unsigned char *)W(SCRATCH, 0x2F4), 2);
        if (abs_val < 0) {
            abs_val = -abs_val;
        }
        if (abs_val >= 0xE01) {
            if (B(SCRATCH, 0x2AD) == 0xFF) {
                actor = ((side_flag & 0xFFU) != 0) ? (D_800FF748 + 0x3E8) : (D_800FF748 + 0x2EE0);
                B(SCRATCH, 0x2AD) = func_8007BA04(actor);
            }
            goto check_wall_count;
        }
    }

    if (B(SCRATCH, 0x2AD) == 0xFF) {
        B(SCRATCH, 0x2AD) = wall_ids[0];
        abs_val = (short)ball_x;
        if (abs_val < 0) {
            abs_val = -abs_val;
        }
        if (abs_val >= 0x23E3) {
            abs_val = (short)ball_z;
            if (abs_val < 0) {
                abs_val = -abs_val;
            }
            if (abs_val < 0x12F4) {
                B(SCRATCH, 0x2AD) = func_8005CE14(wall_ids[0]);
            }
        }
    }

check_wall_count:
    if ((wall_count & 0xFFU) >= 4U) {
        active_cnt = 0;
        idx = func_8005CE14(B(SCRATCH, 0x2AD)) & 0xFFU;
        actor = D_800FF748 + 0x3E8 + idx * 1000U;
        for (slot_loop = 0; (slot_loop & 0xFFU) < 10U; slot_loop++, actor += 0x3E8) {
            if (B(actor, 0) != 0) {
                active_cnt++;
            }
        }
        if ((active_cnt & 0xFFU) >= 4U) {
            wall_count--;
        }
    }

    if (B(SCRATCH, 0x2E1) == 5) {
        side_flag ^= 1U;
    }
    func_80068A38(wall_ids, (side_flag ^ 1U) & 0xFFU);

    if ((wall_count & 0xFFU) == 0) {
        func_8009F574();
        return;
    }

    if ((side_flag & 0xFFU) != 0) {
        actor = D_800FF748 + 0x3E8;
    } else {
        actor = D_800FF748 + 0x2EE0;
    }

    S(actor, 2) = ((side_flag & 0xFFU) != 1U) ? 0x3220 : -0x3220;

    if ((wall_count & 0xFFU) >= 5U) {
        S(actor, 6) = ((short)ball_z > 0) ? -0x2C5 : 0x2C5;
    }
    if ((wall_count & 0xFFU) >= 4U) {
        S(actor, 6) = ((short)ball_z <= 0) ? 0xBD : -0xBD;
    } else {
        S(actor, 6) = 0;
    }

    kicker = D_800FF748 + (B(SCRATCH, 0x2AD) % 24U) * 1000U;
    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    gk_ang = func_8005B36C(S(ball, 2), S(ball, 6), S(actor, 2), S(actor, 6));

    gk_diff = (gk_ang - (B(kicker, 0x399) << 7)) & 0xFFU;
    if (gk_diff < 0x81U) {
        if ((int)gk_diff >= 0x31) {
            goto adjust_gk_far;
        }
    } else if ((int)(((B(kicker, 0x399) << 7) - gk_ang) & 0xFFU) >= 0x31) {
adjust_gk_far:
        S(actor, 6) = ((short)ball_z > 0) ? 0x13B : -0x276;
        goto gk_done;
    }

    gk_diff = (gk_ang - (B(kicker, 0x399) << 7)) & 0xFFU;
    if (gk_diff < 0x81U) {
        if ((int)gk_diff >= 0x21) {
            goto adjust_gk_mid;
        }
    } else if ((int)(((B(kicker, 0x399) << 7) - gk_ang) & 0xFFU) >= 0x21) {
adjust_gk_mid:
        S(actor, 6) = ((short)ball_z > 0) ? 0x13B : -0x13B;
    }

gk_done:
    if (B(SCRATCH, 0x2E1) == 5) {
        goal_x = ((side_flag & 0xFFU) != 1U) ? -0x3320 : 0x3320;
    } else {
        goal_x = ((side_flag & 0xFFU) != 0) ? -0x3320 : 0x3320;
    }

    goal_z = ((short)ball_z <= 0) ? 0x1D9 : -0x1D9;
    switch ((wall_count & 0xFFU) - 1U) {
    case 0:
        goal_z = 0;
        break;
    case 1:
        goal_z = ((short)ball_z <= 0) ? -0x3F2 : 0x3F2;
        abs_bz = S((unsigned char *)W(SCRATCH, 0x2F4), 6);
        if (abs_bz < 0) {
            abs_bz = -abs_bz;
        }
        if (abs_bz >= 0x12F5) {
            goal_z = ((short)ball_z <= 0) ? -0x4B2 : 0x4B2;
        }
        break;
    case 2:
        abs_bz = (short)ball_z;
        if (abs_bz < 0) {
            abs_bz = -abs_bz;
        }
        if (abs_bz >= 0x393) {
            goal_z = ((short)ball_z <= 0) ? -0x472 : 0x472;
        } else {
            goal_z = ((short)ball_z <= 0) ? -0x4C2 : 0x4C2;
        }
        break;
    case 3:
        abs_bz = (short)ball_z;
        if (abs_bz < 0) {
            abs_bz = -abs_bz;
        }
        if (abs_bz >= 0x393) {
            goal_z = ((short)ball_z <= 0) ? -0x492 : 0x492;
        } else {
            goal_z = ((short)ball_z <= 0) ? -0x4F2 : 0x4F2;
        }
        break;
    default:
        break;
    }

    first_wall = D_800FF748 + (wall_ids[0] % 24U) * 1000U;
    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    wall_ang = func_8005B36C(S(ball, 2), S(ball, 6), (short)goal_x, (short)goal_z);

    S(first_wall, 2) = (short)((short)ball_x + func_8001E694(wall_ang & 0xFFU, 0x928));
    S(first_wall, 6) = (short)((short)ball_z + func_8001E5A4(wall_ang & 0xFFU, 0x928));
    B(first_wall, 0x396) = 0x35;
    B(first_wall, 0x397) = 0;

    if (B(SCRATCH, 0x2E1) == 5) {
        side_flag ^= 1U;
    }

    if ((side_flag & 0xFFU) == 0) {
        if ((short)ball_z <= 0) {
            perp_raw = (wall_ang & 0xFFU) + 0x40U;
            perp_mod = ((int)perp_raw >= 0) ? (int)perp_raw : (int)((wall_ang & 0xFFU) + 0x13FU);
        } else {
            perp_raw = (wall_ang & 0xFFU) + 0xC0U;
            perp_mod = ((int)perp_raw >= 0) ? (int)perp_raw : (int)((wall_ang & 0xFFU) + 0x1BFU);
        }
    } else if ((side_flag & 0xFFU) == 1U) {
        if ((short)ball_z <= 0) {
            perp_raw = (wall_ang & 0xFFU) + 0xC0U;
            perp_mod = ((int)perp_raw >= 0) ? (int)perp_raw : (int)((wall_ang & 0xFFU) + 0x1BFU);
        } else {
            perp_raw = (wall_ang & 0xFFU) + 0x40U;
            perp_mod = ((int)perp_raw >= 0) ? (int)perp_raw : (int)((wall_ang & 0xFFU) + 0x13FU);
        }
    } else {
        perp_raw = (wall_ang & 0xFFU) + 0xC0U;
        perp_mod = ((int)perp_raw >= 0) ? (int)perp_raw : (int)((wall_ang & 0xFFU) + 0x1BFU);
    }
    perp_ang = (perp_raw - (unsigned int)(perp_mod & 0x300)) & 0xFFU;

    if ((wall_count & 0xFFU) > 1U) {
        for (i = 1; (i & 0xFFU) < (wall_count & 0xFFU); i++) {
            unsigned int wid = wall_ids[i & 0xFFU];
            if (wid == 0xFFU) {
                break;
            }
            spacing = (int)((i & 0xFFU) * 185U);
            actor = D_800FF748 + (wid % 24U) * 1000U;
            S(actor, 2) = (short)(H(first_wall, 2) + func_8001E694(perp_ang, spacing));
            S(actor, 6) = (short)(H(first_wall, 6) + func_8001E5A4(perp_ang, spacing));
            B(actor, 0x396) = 0x35;
            B(actor, 0x397) = 0;
        }
    }

    func_8009F574();
}
