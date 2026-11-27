/*
 * func_801B9574 (overlay: select)
 *
 * Purpose:
 *   Initializes and populates the match/post-match summary box UI in the `select`
 *   overlay (background panel `D_801D8D90`, sprite slots 4..6, text/label/stat
 *   entries in buffers `0x30F0` and `0x30F4`). Depending on `arg0` (0..3), writes
 *   team stats (`D_800FF748 + 0xABA2`, `0xABA4`, `0xABA5`) and Scratchpad match
 *   score/half/extra-time/PK fields (`0x1F8002D1..0x1F8002DF`), and disables or
 *   updates trailing `0x30F4` entry flags (`+0xA = 0x5B`).
 *
 * Arguments:
 *   arg0 (u8 in $a0): display mode (0..3)
 *
 * Return value:
 *   None (void)
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef signed short   s16;
typedef unsigned int   u32;
typedef signed int     s32;

typedef struct {
    s16 halves[4];  /* 0x00..0x07: x, y, w, h */
    u8  bytes[12];  /* 0x08..0x13: color / style bytes */
} Fields20; /* sizeof == 0x14 */

/* PS1 Scratchpad RAM (0x1F800000..0x1F8003FF) */
#define SPAD_GOALS          ((volatile u8 *)0x1F8002D1) /* [0]=team0, [1]=team1 */
#define SPAD_1H_GOALS(team) (*(volatile u8 *)(0x1F8002D3U + ((u32)(team) << 2))) /* 0x2D3 / 0x2D7 */
#define SPAD_2H_GOALS(team) (*(volatile u8 *)(0x1F8002D4U + ((u32)(team) << 2))) /* 0x2D4 / 0x2D8 */
#define SPAD_ET1_GOALS(team) (*(volatile u8 *)(0x1F8002D5U + ((u32)(team) << 2))) /* 0x2D5 / 0x2D9 */
#define SPAD_ET2_GOALS(team) (*(volatile u8 *)(0x1F8002D6U + ((u32)(team) << 2))) /* 0x2D6 / 0x2DA */
#define SPAD_PK_GOALS(team) (*(volatile u8 *)(0x1F8002DBU + (u32)(team)))         /* 0x2DB / 0x2DC */
#define SPAD_MATCH_END_TYPE (*(volatile u8 *)0x1F8002DF)

extern u8       D_800FF748[]; /* base symbol for select/work RAM offsets */
extern u8       D_8010865D;   /* side/home-away toggle (0 or 1) */
extern Fields20 D_801D8D90;   /* background quad/panel descriptor */

/* Offsets relative to D_800FF748 */
#define D_8010A2EA ((volatile u8 *)(D_800FF748 + 0xABA2)) /* [2]: per-team stat byte (-0x545E) */
#define D_8010A2EC ((volatile u8 *)(D_800FF748 + 0xABA4)) /* [2][2]: per-team half/sub-stat byte 0 (-0x545C) */
#define D_8010A2ED ((volatile u8 *)(D_800FF748 + 0xABA5)) /* [2][2]: per-team half/sub-stat byte 1 (-0x545B) */

extern u8  *func_8001D004(u32 id);
extern void func_80193FB4(u8 index, u16 key, u32 word8, u16 half16, u16 half18,
                          u8 byte20, u8 byte21, u16 half30, u16 half32,
                          u16 half26, u16 half28, u8 byte22, u8 byte23, u8 byte24,
                          u32 word36, u8 word0, u8 byte4);
extern void func_801941E0(u8 index, u32 word, Fields20 *source, u8 byte2, u16 half0, u8 byte3);
extern u8  *func_801A9540(u8 *entry, u32 val, u32 col, u32 arg3, u32 style);
extern u8  *func_801A9614(u8 *entry, u32 val, u32 col, u32 style);

void func_801B9574(u8 arg0)
{
    u8 mode = arg0;
    u8 side_toggle = D_8010865D;
    u8 team0;
    u8 team1;
    u8 *p;
    u8 *s4 = 0; /* UNCLEAR: left uninitialized in asm if mode >= 4 */
    u8 style_tail;
    u8 tail_flag;
    s32 i;

    D_801D8D90.halves[0] = 0;
    D_801D8D90.halves[1] = 0x14;
    D_801D8D90.halves[2] = 0x7A;
    D_801D8D90.halves[3] = 0x68;
    D_801D8D90.bytes[0] = 0;
    D_801D8D90.bytes[1] = 0;
    D_801D8D90.bytes[2] = 0x20;

    team0 = (u8)((mode >= 2U ? 1U : 0U) ^ side_toggle);
    team1 = (u8)(team0 ^ 1U);

    func_801941E0(0, 0x60000000U, &D_801D8D90, 0, 1, 1);

    func_80193FB4(4, 0x30F1, 0, 0, 0, 0x1A, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 2, 1);
    func_80193FB4(5, 0x30F0, 0, 0, 0, 0x1A, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 2, 1);
    func_80193FB4(6, 0x30F4, 0, 0, 0, 0x1D, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 2, 1);

    switch (mode) {
    case 0:
        p = func_8001D004(0x30F0);
        p = func_801A9540(p, 0, 2, 0xB0, 0x61);
        p = func_801A9540(p, 0, 1, 0xB0, 0x61);
        p = func_801A9540(p + 0xC, 0, 2, 0xB0, 0x5B);
        func_801A9540(p, 0, 1, 0xB0, 0x5B);

        p = func_8001D004(0x30F4);
        p = func_801A9614(p, 0, 2, 0x50);
        p = func_801A9614(p, 0, 1, 0x50);
        p = func_801A9614(p, 0, 2, 0x50);
        p = func_801A9614(p, 0, 1, 0x50);
        p = func_801A9614(p, 0, 2, 0x5B);
        p = func_801A9614(p, 0, 1, 0x5B);
        p = func_801A9614(p, 0, 2, 0x5B);
        s4 = func_801A9614(p, 0, 1, 0x5B);
        break;

    case 1:
    case 2:
        style_tail = (mode == 1) ? 0x5BU : 0x61U;

        p = func_8001D004(0x30F0);
        p = func_801A9540(p, D_8010A2EA[team0], 2, 0xB0, 0x61);
        p = func_801A9540(p, D_8010A2EA[team1], 1, 0xB0, 0x61);
        p = func_801A9540(p + 0xC, 0, 2, 0xB0, style_tail);
        func_801A9540(p, 0, 1, 0xB0, style_tail);

        p = func_8001D004(0x30F4);
        p = func_801A9614(p, D_8010A2EC[team0 * 2], 2, 0x50);
        p = func_801A9614(p, D_8010A2EC[team1 * 2], 1, 0x50);
        p = func_801A9614(p, D_8010A2ED[team0 * 2], 2, 0x50);
        p = func_801A9614(p, D_8010A2ED[team1 * 2], 1, 0x50);
        p = func_801A9614(p, 0, 2, style_tail);
        p = func_801A9614(p, 0, 1, style_tail);
        p = func_801A9614(p, 0, 2, style_tail);
        s4 = func_801A9614(p, 0, 1, style_tail);
        break;

    case 3:
        p = func_8001D004(0x30F0);
        p = func_801A9540(p, D_8010A2EA[team0], 2, 0xB0, 0x61);
        p = func_801A9540(p, D_8010A2EA[team1], 1, 0xB0, 0x61);
        p = func_801A9540(p + 0xC, SPAD_GOALS[team0], 2, 0xB0, 0x61);
        func_801A9540(p, SPAD_GOALS[team1], 1, 0xB0, 0x61);

        p = func_8001D004(0x30F4);
        p = func_801A9614(p, D_8010A2EC[team0 * 2], 2, 0x50);
        p = func_801A9614(p, D_8010A2EC[team1 * 2], 1, 0x50);
        p = func_801A9614(p, D_8010A2ED[team0 * 2], 2, 0x50);
        p = func_801A9614(p, D_8010A2ED[team1 * 2], 1, 0x50);
        p = func_801A9614(p, SPAD_1H_GOALS(team0), 2, 0x50);
        p = func_801A9614(p, SPAD_1H_GOALS(team1), 1, 0x50);
        p = func_801A9614(p, SPAD_2H_GOALS(team0), 2, 0x50);
        s4 = func_801A9614(p, SPAD_2H_GOALS(team1), 1, 0x50);
        break;

    default:
        break;
    }

    if (mode == 3 && SPAD_MATCH_END_TYPE >= 2U && SPAD_MATCH_END_TYPE < 6U) {
        if (SPAD_MATCH_END_TYPE < 4U) {
            /* Extra time goals: sum of ET1 + ET2 halves */
            p = func_801A9614(s4, (u32)SPAD_ET1_GOALS(team0) + (u32)SPAD_ET2_GOALS(team0), 2, 0x50);
            s4 = func_801A9614(p, (u32)SPAD_ET1_GOALS(team1) + (u32)SPAD_ET2_GOALS(team1), 1, 0x50);
            s4[0xA] = 0x50;
            s4[0x3] = 0;
            s4[0x4] = 0x38;
        } else {
            /* Penalty shootout goals */
            p = func_801A9614(s4, SPAD_PK_GOALS(team0), 2, 0x50);
            s4 = func_801A9614(p, SPAD_PK_GOALS(team1), 1, 0x50);
            s4[0xA] = 0x50;
            s4[0x3] = 0x20;
            s4[0x4] = 0x08;
        }
        tail_flag = 0x5B;
    } else {
        p = func_801A9614(s4, 0, 2, 0x5B);
        p = func_801A9614(p, 0, 1, 0x5B);
        p[0xA] = 0x5B;
        tail_flag = 0x61;
    }

    p = func_8001D004(0x30F0);
    p[0x76] = tail_flag;

    if (mode == 0) {
        p = func_8001D004(0x30F4);
        for (i = 7; i >= 0; i--) {
            p[i * 0xC + 0xA] = 0x5B;
        }
        /* Falls through to clear slots 8..20 in 0x30F4 */
    }

    if (mode == 0 || mode == 1 || mode == 2) {
        p = func_8001D004(0x30F4);
        for (i = 0xC; i >= 0; i--) {
            p[0x60 + i * 0xC + 0xA] = 0x5B;
        }
    }
}
