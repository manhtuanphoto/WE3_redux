/*
 * func_801AA7C4 (overlay: select)
 *
 * Purpose:
 *   Generates simulated match statistics and events (scoreline split across
 *   halves, goal scorers, assisters, and yellow/red cards) for two teams based
 *   on a fixture/result record returned by func_801AAE5C(arg0).
 *   Stores match score/half-goal totals in PS1 Scratchpad (0x1F8002D1..0x1F8002D8)
 *   and player/event tables relative to D_800FF748.
 *
 * Arguments:
 *   arg0 (u8 in $a0): fixture/mode index; when arg0 == 2, skips card generation
 *
 * Return value:
 *   None (void)
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef signed int     s32;
typedef unsigned int   u32;

/* PS1 Scratchpad RAM (0x1F800000..0x1F8003FF) */
#define SPAD_TEAM0_GOALS    (*(volatile u8 *)0x1F8002D1)
#define SPAD_TEAM1_GOALS    (*(volatile u8 *)0x1F8002D2)
#define SPAD_TEAM0_1H_GOALS (*(volatile u8 *)0x1F8002D3)
#define SPAD_TEAM0_2H_GOALS (*(volatile u8 *)0x1F8002D4)
#define SPAD_TEAM1_1H_GOALS (*(volatile u8 *)0x1F8002D7)
#define SPAD_TEAM1_2H_GOALS (*(volatile u8 *)0x1F8002D8)
#define SPAD_TEAM_IDS       ((volatile u8 *)0x1F8002DD) /* [0]=team0 (0x2DD), [1]=team1 (0x2DE) */
#define SPAD_GOALS          ((volatile u8 *)0x1F8002D1) /* [0]=team0, [1]=team1 */

extern u8  D_800FF748[];    /* base symbol for match/player stat arrays */
extern u16 D_8011CB94[];    /* packed 4-nibble scorer slot table per team (stride 4 bytes) */
extern u16 D_8011CB96[];    /* packed 4-nibble scorer weight table per team (stride 4 bytes) */
extern u8  D_80108616;      /* team 0 red card count (D_800FF748 + 0x8ECE) */
extern u8  D_80108617;      /* team 1 red card count (D_800FF748 + 0x8ECF) */
extern u8  D_80108618;      /* team 0 yellow card count (D_800FF748 + 0x8ED0) */
extern u8  D_80108619;      /* team 1 yellow card count (D_800FF748 + 0x8ED1) */

/* Derived buffers inside D_800FF748 */
#define BUF_CARDS_TEAM0     (D_800FF748 + 0x88A5)   /* 22 bytes: card flags per player */
#define BUF_CARDS_TEAM1     (D_800FF748 + 0x88BB)   /* 22 bytes */
#define BUF_GOALS_TEAM0     (D_800FF748 + 0x88D1)   /* 22 bytes: per-player stat/tally */
#define BUF_GOALS_TEAM1     (D_800FF748 + 0x88E7)   /* 22 bytes */
#define BUF_GOAL_EVENTS     (D_800FF748 + 0x88FD)   /* 2 teams * 32 slots * 3 bytes (0x88FD, 0x895D) */
#define BUF_RED_COUNTS      (D_800FF748 + 0x8ECE)   /* 2 bytes: D_80108616..80108617 */
#define BUF_YELLOW_COUNTS   (D_800FF748 + 0x8ED0)   /* 2 bytes: D_80108618..80108619 */

extern void *jtbl_8018A2C0[];

extern u8  *func_801AAE5C(u32 arg0);
extern s32  func_8001E6B8(s32 range);

void func_801AA7C4(u8 arg0)
{
    u8 *match_rec;
    u8 team0_goals;
    u8 team1_goals;
    u32 i;
    u32 team;
    u32 g;
    u8 scorers[4];
    u8 weights[4];

    match_rec = func_801AAE5C((u32)arg0);

    /* Determine goals for team 0 (s1) and team 1 (s0) based on match_rec[0] and team IDs */
    team1_goals = match_rec[3];
    if (match_rec[0] == 3) {
        team0_goals = team1_goals;
    } else if (match_rec[1] == SPAD_TEAM_IDS[0]) {
        team0_goals = match_rec[3];
        team1_goals = match_rec[4];
    } else {
        team0_goals = match_rec[4];
        if (match_rec[1] == SPAD_TEAM_IDS[1]) {
            team1_goals = match_rec[3];
        }
    }

    /* Randomly split each team's goals between 1st half and 2nd half */
    SPAD_TEAM0_1H_GOALS = (u8)func_8001E6B8((s32)team0_goals);
    SPAD_TEAM1_1H_GOALS = (u8)func_8001E6B8((s32)team1_goals);
    SPAD_TEAM0_GOALS = team0_goals;
    SPAD_TEAM1_GOALS = team1_goals;
    SPAD_TEAM0_2H_GOALS = (u8)(team0_goals - SPAD_TEAM0_1H_GOALS);
    SPAD_TEAM1_2H_GOALS = (u8)(team1_goals - SPAD_TEAM1_1H_GOALS);

    /* Clear 22-byte per-player goal/stat tables for both teams */
    for (i = 0; i < 22; i++) {
        BUF_GOALS_TEAM1[i] = 0;
        BUF_GOALS_TEAM0[i] = 0;
    }

    /* Reset 32 goal event slots (3 bytes each) per team to 0xFF */
    for (i = 0; i < 32; i++) {
        u8 *slot0 = BUF_GOAL_EVENTS + i * 3;
        u8 *slot1 = BUF_GOAL_EVENTS + 0x60 + i * 3;
        slot1[2] = 0xFF;
        slot1[1] = 0xFF;
        slot1[0] = 0xFF;
        slot0[2] = 0xFF;
        slot0[1] = 0xFF;
        slot0[0] = 0xFF;
    }

    /* Assign goal scorers and assisters for both teams */
    for (team = 0; team < 2; team++) {
        u8 team_id = SPAD_TEAM_IDS[team];
        u8 goal_count;

        for (i = 0; i < 4; i++) {
            /* UNCLEAR: D_8011CB94/D_8011CB96 indexed by team_id * 2 (4-byte stride) */
            scorers[i] = (u8)((D_8011CB94[team_id * 2] >> (i * 4)) & 0xF);
            weights[i] = (u8)((D_8011CB96[team_id * 2] >> (i * 4)) & 0xF);
        }

        goal_count = SPAD_GOALS[team];
        if (goal_count != 0) {
            u8 *team_goal_buf = BUF_GOALS_TEAM0 + team * 22;

            for (g = 0; g < goal_count; g++) {
                u8 r = (u8)func_8001E6B8(11);
                u32 rem = 10;
                u8 scorer = 0xFF;
                u8 *event;
                u32 extra;

                for (i = 0; i < 4; i++) {
                    if (weights[i] != 0) {
                        rem -= weights[i];
                        if ((u8)rem < r) {
                            scorer = scorers[i];
                            break;
                        }
                    }
                }

                if (scorer == 0xFF) {
                    scorer = (u8)(func_8001E6B8(10) + 1);
                }

                event = BUF_GOAL_EVENTS + (team * 32 + g) * 3;
                event[0] = scorer;
                event[2] = (u8)(func_8001E6B8(111) & 1);

                /* UNCLEAR: Exact meaning of adding (1 + rand(5)) to BUF_GOALS_TEAMx[scorer] */
                extra = (u32)func_8001E6B8(5);
                team_goal_buf[scorer] = (u8)(team_goal_buf[scorer] + 1 + extra);
            }
        }

        goal_count = SPAD_GOALS[team];
        if (goal_count != 0) {
            for (g = 0; g < goal_count; g++) {
                u8 r = (u8)func_8001E6B8(12);
                u8 assister;
                u8 *event;

                if (r < 2) {
                    continue;
                } else if (r < 3) {
                    assister = (u8)(func_8001E6B8(10) + 1);
                } else {
                    assister = scorers[func_8001E6B8(4) & 0xFF];
                }

                event = BUF_GOAL_EVENTS + (team * 32 + g) * 3;
                if (assister == event[0]) {
                    event[1] = (u8)(assister - 1);
                } else {
                    event[1] = assister;
                }
            }
        }
    }

    if (arg0 == 2) {
        return;
    }

    /* Clear 22-byte per-player card tables and total card counts */
    for (i = 0; i < 22; i++) {
        BUF_CARDS_TEAM1[i] = 0;
        BUF_CARDS_TEAM0[i] = 0;
    }

    D_80108618 = 0;
    D_80108619 = 0;
    D_80108616 = 0;
    D_80108617 = 0;

    /* Simulate yellow and red cards for both teams */
    for (team = 0; team < 2; team++) {
        u32 card_rng = (u32)func_8001E6B8(8);
        u32 card_idx;
        u8 *team_cards;

        /* UNCLEAR: jtbl_8018A2C0 (8 entries) not included in compact asm;
         * targets are .L801AACC0 (0), .L801AACC8 (1), and .L801AACD0 (2).
         */
        if (card_rng < 8) {
            /* Dispatches via jtbl_8018A2C0[card_rng] to set card_idx = 0, 1, or 2 */
            card_idx = 2; /* UNCLEAR: placeholder for jtbl_8018A2C0[card_rng] */
        } else {
            card_idx = 2;
        }

        team_cards = BUF_CARDS_TEAM0 + team * 22;
        while ((s32)(card_idx & 0xFF) < func_8001E6B8(4)) {
            u8 player = (u8)(func_8001E6B8(10) + 1);
            u8 cur = team_cards[player];

            if (cur == 0) {
                team_cards[player] = 1;
            } else if (cur < 3) {
                team_cards[player] = (u8)(cur << 1);
            }

            BUF_YELLOW_COUNTS[team]++;
            card_idx++;
        }

        /* 10% chance of a straight red card (bit 2 = 0x4) */
        if (func_8001E6B8(100) < 10) {
            u8 player = (u8)(func_8001E6B8(10) + 1);
            u8 cur = team_cards[player];

            if ((cur & 0x4) == 0) {
                team_cards[player] = (u8)(cur | 0x4);
            }
            BUF_RED_COUNTS[team]++;
        }
    }
}
