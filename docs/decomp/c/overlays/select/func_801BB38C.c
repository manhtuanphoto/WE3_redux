/*
 * func_801BB38C (overlay: select)
 *
 * Purpose:
 *   Main per-frame state-machine dispatcher for the league/tournament standings,
 *   schedule, and progression screen in the `select` overlay. Polls pad 0 into
 *   `D_8010A5A4` and dispatches on `D_8010A322` (u8, states 0x00..0x90 via
 *   `jtbl_8018B630`; >= 0x91 is a no-op). Handles fade/transition setup, match
 *   result recording from Scratchpad (`0x1F8002D1..0x1F8002D8`) into
 *   `D_801D94D8` / `D_801DA310`..`D_801DA311`, sliding panel animations
 *   (`D_801D8D78`, `D_801D92A8`/`D_801D92AC`, `D_801D92C4`/`D_801D92C8`),
 *   scrollable table navigation (`D_801DA5D0..D_801DA5D6`, `func_801BFB24`),
 *   sub-menu/selection calls (`func_801AE294`, `func_801ACE2C`, `func_801AB6E8`),
 *   next-round advancement, and RGB fade-out on `D_80105508[0x37..0x39]`.
 *
 * Arguments:
 *   None (void)
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
    u32 word0;
    u8  byte4;
    u8  padding5;
    u16 half6;
    u32 word8;
    u8 *pointer12;
    u16 half16, half18;
    u8  byte20, byte21, byte22, byte23, byte24, padding25;
    u16 half26, half28, half30, half32, padding34;
    u32 word36;
} Descriptor40; /* sizeof == 0x28 */

typedef struct {
    s16 x;          /* 0x00 */
    s16 y;          /* 0x02 */
    s16 w;          /* 0x04 */
    s16 h;          /* 0x06 */
    u8  rgba[12];   /* 0x08..0x13 */
} MenuBoxDesc; /* sizeof == 0x14 */

/* PS1 Scratchpad RAM (0x1F800000..0x1F8003FF) */
#define SPAD_U8(off) (*(volatile u8 *)(0x1F800000U + (off)))

/* Jump table in .rodata (raw entries not present in compact .s) */
extern void *jtbl_8018B630[];

extern u8           D_800FF748[]; /* base symbol for select/work RAM offsets */
extern s16          D_800AD638;   /* cursor index */
extern s16          D_8010A5A4;   /* last polled button mask from pad 0 */
extern Descriptor40 D_80105508[]; /* UI primitive table at D_800FF748 + 0x5DC0 */

extern u8          D_801D88F0;    /* player rank / end-of-competition dialog ID */
extern u8          D_801D8B18;
extern MenuBoxDesc D_801D8D78;    /* sliding panel descriptor */
extern u8          D_801D8DFB;
extern u8          D_801D8DFC;
extern u8          D_801D92A3;
extern s16         D_801D92A8;    /* left/top panel slide coord 0 */
extern s16         D_801D92AC;    /* left/top panel slide coord 1 */
extern u8          D_801D92BF;
extern s16         D_801D92C4;    /* right/bottom panel slide coord 0 */
extern s16         D_801D92C8;    /* right/bottom panel slide coord 1 */
extern u8          D_801D92DB;
extern u8          D_801D94D8[];  /* recorded match total goals per fixture/side */
extern u32         D_801DA058;    /* animation / step counter */
extern u32         D_801DA05C;
extern u8          D_801DA068;
extern u8          D_801DA069;
extern u8          D_801DA06A;
extern u8          D_801DA06B;
extern u8          D_801DA2F0;    /* active/ready flag */
extern u8          D_801DA2F8;
extern u8          D_801DA310[];  /* recorded 1st-half goals (even bytes) */
extern u8          D_801DA311[];  /* recorded 2nd-half goals (odd bytes, +1) */
extern u8          D_801DA332;
extern u8          D_801DA342;
extern s16         D_801DA5D0;    /* scroll offset */
extern s16         D_801DA5D2;    /* selected row */
extern s16         D_801DA5D4;    /* visible row count (0xC) */
extern s16         D_801DA5D6;    /* total row count (0x10) */
extern u8          D_801DAD85;    /* completion / exit flag */
extern u8          D_801DAE50;    /* dialog result (1 = cancel, 2 = confirm) */

/* Offsets relative to D_800FF748 */
#define D_8010582C (*(volatile u8 *)(D_800FF748 + 0x60E4))
#define D_80105854 (*(volatile u8 *)(D_800FF748 + 0x610C))
#define D_8010865D (*(volatile u8 *)(D_800FF748 + 0x8F15)) /* -0x70EB: side toggle */
#define D_80108667 (*(volatile u8 *)(D_800FF748 + 0x8F1F)) /* -0x70E1: mode flags */
#define D_80108669 (*(volatile u8 *)(D_800FF748 + 0x8F21)) /* -0x70DF: current slot/team cursor */
#define D_8010866A (*(volatile u8 *)(D_800FF748 + 0x8F22)) /* -0x70DE: round counter */
#define D_8010A298 ((volatile u8 *)(D_800FF748 + 0xAB50))  /* -0x54B0: standings order table */
#define D_8010A2C8 ((volatile u8 *)(D_800FF748 + 0xAB80))  /* -0x5480: slot -> entry index */
#define D_8010A2E8 (*(volatile u8 *)(D_800FF748 + 0xABA0)) /* -0x5460: fixture team 0 entry */
#define D_8010A2E9 (*(volatile u8 *)(D_800FF748 + 0xABA1)) /* -0x545F: fixture team 1 entry */
#define D_8010A2F0 (*(volatile u8 *)(D_800FF748 + 0xABA8)) /* -0x5458: title/clinch status */
#define D_8010A320 (*(volatile u8 *)(D_800FF748 + 0xABD8)) /* -0x5428: sub-state */
#define D_8010A322 (*(volatile u8 *)(D_800FF748 + 0xABDA)) /* -0x5426: primary state */
#define D_8010A32A (*(volatile u8 *)(D_800FF748 + 0xABE2)) /* -0x541E: flags */
/* 36-byte (0x24) per-entry standings/team record fields */
#define TEAM_ID_AT(idx)     (*(volatile u8 *)(D_800FF748 + 0x8F25 + (u32)(idx) * 0x24U)) /* -0x70DB */
#define TEAM_PLAYED_AT(idx) (*(volatile u8 *)(D_800FF748 + 0x8F26 + (u32)(idx) * 0x24U)) /* -0x70DA */
#define TEAM_PTS_AT(idx)    (*(volatile u8 *)(D_800FF748 + 0x8F28 + (u32)(idx) * 0x24U)) /* -0x70D8 */

extern void func_80017178(s32 a0);
extern void func_800171C4(s32 a0);
extern void func_800171FC(s32 a0);
extern s32  func_80019B64(s32 a0);
extern void func_8001DF74(s32 a0, s32 a1);
extern void func_8004FC54(s32 a0);
extern u16  func_8006EC80(s32 pad_port);
extern s32  func_8006F07C(s32 step);
extern s32  func_8006F0DC(s32 step, s32 mode);
extern void func_8006FA84(s32 slot, s32 val);
extern void func_800AB620(s32 sfx_id);
extern void func_8018E538(s32 a0);
extern u32  func_8018E5EC(u32 cur, s32 min_val, s32 max_val);
extern s32  func_8018EA8C(s32 a0, s32 a1);
extern s32  func_8018EAF4(s32 a0, s32 a1, s32 a2);
extern void func_8018EC3C(s32 a0, s32 a1);
extern void func_801940EC(s32 index, s32 word, MenuBoxDesc *box, s32 byte2, s32 half0, s32 byte3);
extern void func_801A9C54(void);
extern void func_801AB290(s32 a0);
extern s32  func_801AB6E8(s32 dialog_id);
extern s32  func_801ACE2C(s32 arg0);
extern void func_801AD7EC(void);
extern s32  func_801AE294(u8 arg0);
extern void func_801BC968(void);
extern u32  func_801BCBA8(void);
extern void func_801BCCD8(void);
extern u8  *func_801BCF80(void);
extern void func_801BD3F0(void);
extern void func_801BD6EC(s32 a0);
extern s32  func_801BD8E8(void);
extern void func_801BE744(void);
extern void func_801BF35C(void);
extern void func_801BFB24(void);

void func_801BB38C(void)
{
    u8 state;
    s32 res;
    u16 pad;

    D_8010A5A4 = (s16)func_8006EC80(0);
    state = D_8010A322;
    if (state >= 0x91U) {
        return;
    }

    /*
     * UNCLEAR: Exact mapping of `state` (0x00..0x90) via `jtbl_8018B630[state]`
     * is located in .rodata and not present in the compact .s file. Each
     * jump-table target block from the assembly is preserved below.
     */
    (void)jtbl_8018B630[state];
    return;

L_801BB3F0:
    func_801BC968();
    func_801BCBA8();
    func_8018E538(0);
    SPAD_U8(0x2A2) = 0x20;
    func_801BD6EC(0);
    D_800AD638 = 0;
    D_801DA2F8 = 0;
    D_801DA2F0 = 0;
    D_801DAD85 = 0;
    D_801DA342 = 0x3C;
    D_801DA332 = 0;
    D_8010A322++;
    return;

L_801BB460:
    res = func_8006F07C(0x10);
    if (res != 0) {
        D_8010A322++;
    }
    return;

L_801BB47C:
    if (func_801BD8E8() != 0) {
        D_8010A322++;
    }
    return;

L_801BB494:
    if (D_801DA342 != 0) {
        D_801DA342--;
        D_801DA2F0 = 0;
    } else {
        D_801DA2F0 = 1;
        D_801DA332++;
    }
    res = func_8018EA8C(1, 0);
    D_8010A322 = (u8)(D_8010A322 + res);
    return;

L_801BB4F4:
    if (func_8006F0DC(0x10, 1) != 0) {
        D_8010A322++;
    }
    return;

L_801BB510: {
    u8 *match_rec = func_801BCF80();
    u8 pts0 = TEAM_PTS_AT(match_rec[0]);
    u8 pts1 = TEAM_PTS_AT(match_rec[2]);
    u8 flags = D_80108667;
    u8 round = D_8010866A;

    D_8010A2F0 = 0;
    if ((flags & 0x40U) ? (round == 0x1DU) : (round == 0x0EU)) {
        if ((s32)pts0 - (s32)pts1 >= 4) {
            D_8010A2F0 = (match_rec[0] == D_80108669) ? 2 : 1;
        }
    }
    func_801AB290(0);
    return;
}

L_801BB5F0: {
    u8 idx;
    u8 side;

    if (func_8006F0DC(0x10, 1) == 0) {
        return;
    }

    D_801D8B18 = 0;
    func_801BC968();
    func_801A9C54();
    idx = (u8)func_801BCBA8();
    side = D_8010865D;

    D_801D94D8[idx * 2U + side] = SPAD_U8(0x2D1);
    D_801D94D8[idx * 2U + (side ^ 1U)] = SPAD_U8(0x2D2);
    D_801DA310[idx * 4U + side * 2U] = SPAD_U8(0x2D3);
    D_801DA310[idx * 4U + (side ^ 1U) * 2U] = SPAD_U8(0x2D7);
    D_801DA311[idx * 4U + side * 2U] = SPAD_U8(0x2D4);
    D_801DA311[idx * 4U + (side ^ 1U) * 2U] = SPAD_U8(0x2D8);

    TEAM_PLAYED_AT(D_8010A2E8)++;
    TEAM_PLAYED_AT(D_8010A2E9)++;

    func_801BCCD8();

    D_801DA5D4 = 0xC;
    D_801DA06B = 0;
    D_801DA069 = 0;
    D_801DA06A = 0;
    D_801DA068 = 0;
    D_801DA5D0 = 0;
    D_801DA5D2 = 0;
    D_801DA5D6 = 0x10;
    D_800AD638 = 0;
    D_801DA2F8 = 0;
    D_801DA2F0 = 0;

    func_801BCF80();
    D_8010A322++;
    return;
}

L_801BB800:
    func_801BC968();
    func_8018E538(0);
    SPAD_U8(0x2A2) = 0x21;
    func_801BD6EC(1);
    D_801DA342 = 0x3C;
    D_801DA332 = 0;
    D_8010A322++;
    return;

L_801BB848:
    if (func_8006F07C(0x40) != 0) {
        D_8010A322++;
    }
    return;

L_801BB850:
    if (D_801DA342 != 0) {
        D_801DA342--;
        D_801DA2F0 = 0;
    } else {
        D_801DA2F0 = 1;
        D_801DA332++;
    }
    res = func_8018EA8C(1, 0);
    D_8010A322 = (u8)(D_8010A322 + res);
    return;

L_801BB8B0:
    if (func_8006F0DC(0x40, 1) != 0) {
        SPAD_U8(0x2A2) = 0;
        D_801DA05C = 0;
        D_8010A322++;
    }
    return;

L_801BB8E4:
    func_8001DF74(0x180, 0x1E0);
    D_8010A322 = 0x1E;
    return;

L_801BB908:
    D_801DA2F0 = 1;
    D_8010A322++;
    return;

L_801BB928:
    func_801BE744();
    D_8010A322++;
    return;

L_801BB938:
    pad = func_8006EC80(0);
    D_8010A5A4 = (s16)pad;
    if (pad == 0x1000U || pad == 0x4000U) {
        func_800AB620(0x50D);
        D_800AD638 = (s16)func_8018E5EC((u16)D_800AD638, 0, 0xF);
    }
    if (D_801D8DFB == 1U || D_801D8B18 == 1U) {
        res = func_8018EA8C(1, 0);
    } else {
        res = func_8018EAF4(1, 2, 0);
    }
    D_8010A322 = (u8)(D_8010A322 + res);
    return;

L_801BB9C0:
    D_8010A322 = 0x28;
    return;

L_801BB9D8:
    if (func_8006F0DC(0x10, 1) != 0) {
        D_801DA05C = 0;
        D_801DA332 = 0;
        D_8010A322++;
    }
    return;

L_801BBA10:
    func_8018E538(0);
    func_8001DF74(0x180, 0xF0);
    D_801DA2F8 = 0;
    D_8010A322 = 0x15;
    return;

L_801BBA44:
    D_801DA2F0 = 1;
    D_801D8D78.x = 0x90;
    D_801D8D78.y = -0xA1;
    D_801D8D78.w = 0x1C;
    D_801D8D78.h = 0x160;
    D_801DA058 = 1;
    D_801D8D78.rgba[0] = 0;
    D_801D8D78.rgba[1] = 0;
    D_801D8D78.rgba[2] = 0x60;
    D_801D8D78.rgba[3] = 0;
    D_801D8D78.rgba[4] = 0;
    D_801D8D78.rgba[5] = 0;
    func_801940EC(0, 0, &D_801D8D78, 1, 1, 1);
    D_8010A322++;
    return;

L_801BBAE4: {
    s32 step = (s32)D_801DA058;

    D_801D92A8 = (s16)((7 - step) * 0x1C - 0x34);
    D_801D92AC = (s16)(step * 0x1C + 0x1C);
    D_801DA058 = (u32)(step + 1);
    if (step >= 7) {
        D_8010A322++;
    }
    return;
}

L_801BBB44:
    SPAD_U8(0x2A2) = 0x23;
    func_8006FA84(0x14, 0);
    func_8006FA84(0x15, 1);
    D_801D8D78.x = -0x34;
    D_801D8D78.y = -0xA1;
    D_801D8D78.w = 0xE0;
    D_801D8D78.h = 0x160;
    func_801940EC(0, 0, &D_801D8D78, 1, 5, 1);
    D_8010A322++;
    return;

L_801BBBC0:
    D_8010A322 = 0x32;
    return;

L_801BBBD8:
    D_801DA058 = 7;
    SPAD_U8(0x2A2) = 0x22;
    D_8010582C = 1;
    D_80105854 = 0;
    D_801D8D78.x = -0x34;
    D_801D8D78.y = -0xA1;
    D_801D8D78.w = 0xE0;
    D_801D8D78.h = 0x160;
    func_801940EC(0, 0, &D_801D8D78, 1, 1, 1);
    D_8010A322++;
    return;

L_801BBC44: {
    s32 step = (s32)D_801DA058;

    D_801D92A8 = (s16)((7 - step) * 0x1C - 0x34);
    D_801D92AC = (s16)(step * 0x1C + 0x1C);
    step--;
    D_801DA058 = (u32)step;
    if (step == -1) {
        D_801D92A3 = 0;
        D_8010A322 = 0x21;
    }
    return;
}

L_801BBCB8:
    D_801DA2F0 = 1;
    func_801BD3F0();
    D_801D8D78.x = 0x90;
    D_801D8D78.y = -0xA1;
    D_801D8D78.w = 0x1C;
    D_801D8D78.h = 0x160;
    D_801DA058 = 1;
    D_801D8D78.rgba[0] = 0;
    D_801D8D78.rgba[1] = 0;
    D_801D8D78.rgba[2] = 0x60;
    D_801D8D78.rgba[3] = 0;
    D_801D8D78.rgba[4] = 0;
    D_801D8D78.rgba[5] = 0;
    func_801940EC(1, 0, &D_801D8D78, 1, 1, 1);
    D_8010A322++;
    return;

L_801BBCE8: {
    s32 step = (s32)D_801DA058;

    D_801D92C4 = (s16)((7 - step) * 0x1C - 0x34);
    D_801D92C8 = (s16)(step * 0x1C + 0x1C);
    D_801DA058 = (u32)(step + 1);
    if (step >= 7) {
        D_801DA058 = 7;
        D_8010A322++;
    }
    return;
}

L_801BBD5C:
    SPAD_U8(0x2A2) = 0x24;
    func_8006FA84(0x14, 0);
    func_8006FA84(0x15, 1);
    D_801D8D78.x = -0x34;
    D_801D8D78.y = -0xA1;
    D_801D8D78.w = 0xE0;
    D_801D8D78.h = 0x160;
    func_801940EC(1, 0, &D_801D8D78, 1, 5, 1);
    D_8010A322++;
    return;

L_801BBD84:
    pad = func_8006EC80(0);
    D_8010A5A4 = (s16)pad;
    if (pad == 0x1000U || pad == 0x4000U) {
        func_800AB620(0x50D);
        D_800AD638 = (s16)func_8018E5EC((u16)D_800AD638, 0, 0xF);
    }
    res = func_8018EAF4(1, 2, 0);
    D_8010A322 = (u8)(D_8010A322 + res);
    return;

L_801BBE08:
    if (func_8006F0DC(0x40, 1) != 0) {
        D_8010A322 = 0x3C;
    }
    return;

L_801BBE30:
    SPAD_U8(0x2A2) = 0x23;
    D_801D8D78.x = -0x34;
    D_801D8D78.y = -0xA1;
    D_801D8D78.w = 0xE0;
    D_801D8D78.h = 0x160;
    func_801940EC(1, 0, &D_801D8D78, 1, 1, 1);
    D_8010A322++;
    return;

L_801BBE84: {
    s32 step = (s32)D_801DA058;

    D_801D92C4 = (s16)((7 - step) * 0x1C - 0x34);
    D_801D92C8 = (s16)(step * 0x1C + 0x1C);
    step--;
    D_801DA058 = (u32)step;
    if (step == -1) {
        D_801D92BF = 0;
        D_8010A322 = 0x2A;
    }
    return;
}

L_801BBEF8:
    D_801DA2F0 = 1;
    func_8018E538(0);
    func_801BF35C();
    D_8010A322++;
    return;

L_801BBF1C:
    pad = (u16)D_8010A5A4;
    if (pad == 0x1000U || pad == 0x4000U) {
        func_800AB620(0x50D);
        D_801DA5D2 = (s16)func_8018E5EC((u16)D_801DA5D2, 0, (s32)D_801DA5D6 - 1);
    } else if (pad == 0x20U) {
        D_801DAE50 = 2;
        func_800AB620(0x528);
        D_8010A322++;
    } else if (pad == 0x40U) {
        D_801DAE50 = 1;
        func_800AB620(0x529);
        D_8010A322++;
    } else if (pad == 0x8000U) {
        if (D_801DA5D0 != 0) {
            D_801DA5D0--;
            func_800AB620(0x50D);
        }
    } else if (pad == 0x2000U) {
        if (D_801DA5D0 < 0x10 - D_801DA5D4) {
            D_801DA5D0++;
            func_800AB620(0x50D);
        }
    }
    func_801BFB24();
    return;

L_801BC068:
    if (func_8006F0DC(0x10, 1) == 0) {
        return;
    }
    if (D_801DAE50 == 1U) {
        D_8010A322++;
        return;
    }
    func_8001DF74(0x180, 0xF0);
    D_8010A322 = 0x46;
    return;

L_801BC0A0:
    func_801BE744();
    SPAD_U8(0x2A2) = 0x24;
    D_8010582C = 0;
    D_80105854 = 1;
    D_801D8D78.x = -0x34;
    D_801D8D78.y = -0xA1;
    D_801D8D78.w = 0xE0;
    D_801D8D78.h = 0x160;
    D_801D8D78.rgba[0] = 0;
    D_801D8D78.rgba[1] = 0;
    D_801D8D78.rgba[2] = 0x60;
    D_801D8D78.rgba[3] = 0;
    D_801D8D78.rgba[4] = 0;
    D_801D8D78.rgba[5] = 0;
    func_801940EC(0, 0, &D_801D8D78, 1, 5, 1);
    func_801940EC(1, 0, &D_801D8D78, 1, 5, 1);
    D_801DA2F0 = 1;
    D_8010A322++;
    return;

L_801BC17C:
    if (func_8006F07C(0x10) != 0) {
        D_8010A322 = 0x35;
    }
    return;

L_801BC1A0:
    D_801DA2F0 = 0;
    D_8010A320 = 0;
    D_8010A322++;
    return;

L_801BC1C8:
    res = func_801AE294(0);
    if (res == 1) {
        func_8001DF74(0x180, 0x1E0);
        D_801DA05C = 0;
        D_8010A322 = 0x3C;
    } else if (res == 2) {
        D_8010A322++;
    }
    return;

L_801BC204:
    D_801DA2F0 = 0;
    D_8010A320 = 0;
    D_8010A322++;
    return;

L_801BC22C:
    res = func_801AE294(1);
    if (res == 1) {
        D_8010A322 = 0x46;
    } else if (res == 2) {
        D_8010A322 = 0x50;
    }
    return;

L_801BC26C:
    if (D_801D8B18 == 1U) {
        D_8010A322 = 0x78;
        return;
    }
    if ((D_80108667 & 0x40U) ? (D_8010866A == 0x1DU) : (D_8010866A == 0x0EU)) {
        D_801DAD85 = 1;
        D_8010A322 = 0x78;
    } else {
        D_8010A322++;
    }
    return;

L_801BC2E4:
    D_801D8DFC = 1;
    D_8010A320 = 0;
    D_8010A322++;
    return;

L_801BC310: {
    u8 cur = D_80108669;
    u8 slot = D_8010A2C8[cur];

    D_8010A2E8 = cur;
    SPAD_U8(0x2DD) = TEAM_ID_AT(slot);
    D_8010A322++;
    return;
}

L_801BC37C:
    res = func_801ACE2C((D_801D8DFB != 0) ? 2 : 0);
    if (res == 2) {
        D_8010A322 = 0x55;
    } else if (res == 1) {
        D_801D92DB = 0;
        func_8018EC3C(0, 0);
        D_8010A322 = 0x48;
    } else if (res == 3) {
        SPAD_U8(0x29F) = 0;
        func_800171C4(0xFF);
    }
    return;

L_801BC414:
    SPAD_U8(0x2A2) = 0;
    D_8010A322++;
    return;

L_801BC428:
    D_801D8DFC = 1;
    D_801D8DFB = 0;
    D_8010A322++;
    return;

L_801BC450:
    D_8010866A++;
    D_8010A322 = 0;
    return;

L_801BC480:
    if (D_801D8DFB != 0) {
        D_801DA2F0 = 1;
        SPAD_U8(0x2A2) = 0;
        D_801DA05C = 0;
        D_8010A322 = 0x1A;
    } else {
        D_8010A322 = 0x50;
    }
    return;

L_801BC4D8:
    if (func_8006F07C(0x10) != 0) {
        D_801D8DFB = 0;
        D_8010A322 = 0x50;
    }
    return;

L_801BC504:
    func_8006F07C(0x40);
    return;

L_801BC514:
    SPAD_U8(0x2A2) = 0;
    D_8010A322++;
    return;

L_801BC528:
    if (D_8010A298[0] == D_80108669) {
        D_8010A2F0 = 1;
        D_8010A322++;
    } else {
        D_8010A2F0 = 0;
        D_801DAD85 = 1;
        D_8010A322 = 0x8C;
    }
    return;

L_801BC59C: {
    u8 cur;
    u8 slot;

    if (func_8006F0DC(0x10, 1) == 0) {
        return;
    }
    cur = D_80108669;
    slot = D_8010A2C8[cur];
    SPAD_U8(0x2DD) = TEAM_ID_AT(slot);
    if ((D_80108667 & 0x40U) != 0) {
        D_8010A32A |= 4U;
    }
    D_801DA2F0 = 0;
    D_801DAD85 = 1;
    SPAD_U8(0x2A2) = 0;
    SPAD_U8(0x330) = 0;
    func_800AB620(7);
    D_8010A320 = 0;
    D_8010A322++;
    return;
}

L_801BC670:
    func_801AD7EC();
    return;

L_801BC680:
    D_801DA2F0 = 1;
    SPAD_U8(0x2A2) = 0;
    D_801DA05C = 0;
    D_801DA5D0 = 0;
    D_801DA5D2 = 0;
    D_801DA5D4 = 0xC;
    D_801DA5D6 = 0x10;
    D_800AD638 = 0;
    D_801DA2F8 = 0;
    SPAD_U8(0x29F) = 0;
    D_8010A322++;
    return;

L_801BC6F8:
    if (func_80019B64(0xCA) != 0) {
        D_8010A322++;
    }
    return;

L_801BC710:
    func_800AB620(0x205);
    D_8010A322 = 0x1A;
    return;

L_801BC730: {
    u8 top = D_8010A298[0];
    u8 cur = D_80108669;
    s32 i;
    u8 rank;

    D_801D88F0 = 0;
    if (top != cur) {
        i = 1;
        do {
            D_801D88F0++;
            rank = D_8010A298[i];
            i++;
        } while (rank != cur);
    }

    rank = D_801D88F0;
    if (rank == 1U) {
        D_801D88F0 = 0x55;
    } else if ((u32)(rank - 2U) < 6U) {
        D_801D88F0 = 0x56;
    } else {
        D_801D88F0 = 0x57;
    }

    D_801DA058 = 0;
    D_8010A322++;
    return;
}

L_801BC7E0:
    if ((s32)D_801DA058 < 0x10) {
        D_801DA058++;
        return;
    }
    res = func_801AB6E8(D_801D88F0);
    if (res == 1) {
        func_801AB6E8(-1);
        D_8010A322 = 0x48;
    } else if (res == 2) {
        func_801AB6E8(-1);
        D_801DA058 = 0;
        D_8010A322++;
    }
    return;

L_801BC874: {
    s32 step = (s32)D_801DA058;
    s32 i;

    D_801DA058 = (u32)(step + 1);
    if (step >= 0x40) {
        D_8010A322++;
        return;
    }
    for (i = 0x37; i < 0x3A; i++) {
        D_80105508[i].byte22 = (u8)(D_80105508[i].byte22 - 2U);
        D_80105508[i].byte23 = (u8)(D_80105508[i].byte23 - 2U);
        D_80105508[i].byte24 = (u8)(D_80105508[i].byte24 - 2U);
    }
    return;
}

L_801BC8D8:
    D_8010A322++;
    return;

L_801BC900:
    func_80017178(6);
    func_800171C4(0);
    func_800171FC(0);
    SPAD_U8(0x2A2) = 0;
    func_8004FC54(0);
    func_8018E538(0);
    D_8010A322 = 0;
    D_8010A320 = 0;
    return;

L_801BC944:
    return;
}
