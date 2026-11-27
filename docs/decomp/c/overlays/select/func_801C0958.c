/*
 * func_801C0958 (overlay: select)
 *
 * Purpose:
 *   Top-level state machine for the league/tournament schedule & match setup
 *   flow in the select overlay, dispatching on D_8010A322 (states 0..0x32 via
 *   jtbl_8018B9DC). Coordinates fade-ins/fade-outs, match pairing and round
 *   progression (D_80108668 / D_8010866A / D_8010866B), standings/fixture views
 *   (func_801C16DC, func_801C2A3C, func_801C3150), team selection (func_801AE294),
 *   sub-menu confirmation (func_801ACE2C), and transition to match launch.
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

/* PS1 Scratchpad RAM (0x1F800000..0x1F8003FF) */
#define SPAD_29F (*(volatile u8 *)0x1F80029F)
#define SPAD_2A2 (*(volatile u8 *)0x1F8002A2)
#define SPAD_330 (*(volatile u8 *)0x1F800330)

extern u8  D_800FF748[];    /* base symbol for select/work RAM offsets */
extern u32 D_801234E0[];    /* schedule/table pointer array indexed by D_80108668 */
extern u8  D_801D8B18;
extern u8  D_801D8DFB;
extern u8  D_801D8DFC;
extern u8  D_801DA068;
extern u8  D_801DA069;
extern u8  D_801DA06A;
extern u8  D_801DA06B;
extern u16 D_801DA5D0;
extern u16 D_801DA5D2;
extern u16 D_801DA5D4;
extern u16 D_801DA5D6;
extern u8  D_801DA2F0;
extern u8  D_801DAE50;

/* Base-relative pointers into work RAM (D_800FF748 + offset) */
#define D_80108668 (*(u8  *)(D_800FF748 + 0x8F20)) /* total team count */
#define D_8010866A (*(u8  *)(D_800FF748 + 0x8F22)) /* current match/round index */
#define D_8010866B (*(u8  *)(D_800FF748 + 0x8F23)) /* upper nibble = round/stage index */
#define D_8010866E ((u8   *)(D_800FF748 + 0x8F26)) /* team state table +1 (stride 36 bytes) */
#define D_8010A2E8 (*(u8  *)(D_800FF748 + 0xABA0)) /* home team index */
#define D_8010A2E9 (*(u8  *)(D_800FF748 + 0xABA1)) /* away team index */
#define D_8010A31E (*(s16 *)(D_800FF748 + 0xABD6)) /* fade level (0..0xFF) */
#define D_8010A320 (*(u8  *)(D_800FF748 + 0xABD8)) /* sub-state machine index */
#define D_8010A322 (*(u8  *)(D_800FF748 + 0xABDA)) /* this state machine index */

/* Jump table located in .rodata (raw entries not present in compact .s) */
extern void *jtbl_8018B9DC[];

extern void func_800171C4(s32 a0);
extern void func_800171FC(s32 a0);
extern s32  func_80019B64(s32 a0);
extern s32  func_8006F07C(s32 step);
extern s32  func_8006F0DC(s32 step, s32 mode);
extern void func_800AB620(s32 sfx_id);
extern void func_8018E538(s32 a0);
extern void func_801A8CD0(void);
extern void func_801A98F0(u32 table_ptr, s32 match_idx);
extern void func_801A9C54(void);
extern u8   func_801AB6E8(s32 mode);
extern s32  func_801ACE2C(s32 mode);
extern s32  func_801AE294(s32 mode);
extern s32  func_801C16DC(s32 mode);
extern void func_801C19EC(void (*cb)(void));
extern void func_801C1CD4(u32 stage);
extern void func_801C1D94(void);
extern void func_801C2064(void);
extern void func_801C20A8(void);
extern void func_801C2A3C(void);
extern void func_801C3150(void);

void func_801C0958(void)
{
    u8 state = D_8010A322;

    if (state >= 0x33) {
        return;
    }

    /* UNCLEAR: jtbl_8018B9DC (51 entries) is not present in the compact asm.
     * Below are all 28 branch targets (jlabels) referenced by the jump table.
     */
    (void)jtbl_8018B9DC[state];
    return;

L_801C09A0:
    D_801DA2F0 = 1;
    D_801D8B18 = 0;
    if (func_8006F0DC(0x40, 0) == 0) {
        return;
    }
    func_801A98F0(D_801234E0[D_80108668], (s32)D_8010866A - 1);
    func_801A9C54();
    D_8010866E[(s32)D_8010A2E8 * 36] += 1;
    D_8010866E[(s32)D_8010A2E9 * 36] += 1;
    D_801DA06B = 0;
    D_801DA069 = 0;
    D_801DA06A = 0;
    D_801DA068 = 0;
    D_801DA5D0 = 0;
    D_801DA5D2 = 0;
    D_801DA5D4 = 4;
    D_801DA5D6 = 4;
    func_801C1D94();
    D_8010A322++;
    return;

L_801C0AE0:
    func_801C2064();
    func_801C19EC(func_801C2A3C);
    D_8010A320 = 0;
    D_8010A322++;
    return;

L_801C0B18:
    if (func_8006F07C(0x10) != 0) {
        D_8010A322++;
    }
    return;

L_801C0B3C:
    {
        u32 teams = D_80108668;
        u32 total_matches = (u32)((s32)(teams * (teams - 1)) / 2);
        u32 half_teams;

        D_801DAE50 = 2;
        if (D_8010866A != total_matches) {
            D_801DAE50 = func_801AB6E8(0x60);
        }

        half_teams = teams >> 1;
        /* Note: asm triggers break 7 if half_teams == 0 */
        if ((D_8010866A % half_teams) == 0) {
            func_801C1CD4(D_8010866B >> 4);
            D_801DAE50 = func_801AB6E8(0x61);
        }

        if (D_801DAE50 == 2) {
            func_801AB6E8(-1);
            D_8010A322++;
        }
        return;
    }

L_801C0C24:
    {
        s32 res = func_801C16DC(0);
        if (res == 2) {
            func_800AB620(0x528);
            D_8010A322++;
        } else {
            D_801DA2F0 = (u8)res;
        }
        func_801C2A3C();
        return;
    }

L_801C0C80:
    if (func_8006F0DC(0x40, 0) == 0) {
        return;
    }
    /* Note: asm triggers break 7 if (D_80108668 >> 1) == 0 */
    if ((D_8010866A % (D_80108668 >> 1)) == 0) {
        D_8010A322 = 0x0A;
    } else {
        func_8018E538(0);
        D_8010A322 = 0x1F;
    }
    return;

L_801C0CE0:
    D_801DA2F0 = 1;
    func_801C1D94();
    func_801C20A8();
    func_801C19EC(func_801C3150);
    D_8010A322++;
    return;

L_801C0D20:
    if (func_8006F07C(0x10) != 0) {
        D_8010A322++;
    }
    return;

L_801C0D44:
    {
        s32 res = func_801C16DC(1);
        if (res == 1) {
            func_800AB620(0x529);
            D_8010A322++;
        } else if (res == 2) {
            func_800AB620(0x528);
            D_8010A322++;
        } else {
            D_801DA2F0 = 1;
        }
        func_801C3150();
        return;
    }

L_801C0DB8:
    if (func_8006F0DC(0x40, 0) == 0) {
        return;
    }
    if (D_801DAE50 == 1) {
        D_8010A322++;
    } else if (D_801DAE50 == 2) {
        D_8010A322 = 0x14;
    }
    return;

L_801C0E08:
    func_801C2064();
    func_801C19EC(func_801C2A3C);
    D_8010A320 = 0;
    D_8010A322++;
    return;

L_801C0E40:
    if (func_8006F07C(0x10) != 0) {
        D_8010A322 = 4;
    }
    return;

L_801C0E58:
    D_801DA2F0 = 0;
    D_8010A320 = 0;
    D_8010A322++;
    return;

L_801C0E80:
    {
        s32 res = func_801AE294(0);
        if (res == 1) {
            D_8010A322++;
        } else if (res == 2) {
            D_8010A322 = 0x17;
        }
        return;
    }

L_801C0EC0:
    if (func_8006F0DC(0x40, 0) != 0) {
        D_8010A322 = 0x0A;
    }
    return;

L_801C0EDC:
    D_8010A320 = 0;
    D_8010A322++;
    return;

L_801C0EFC:
    {
        s32 res = func_801AE294(1);
        if (res == 1) {
            D_8010A322 = 0x14;
        } else if (res == 2) {
            D_8010A322 = 0x1E;
        }
        return;
    }

L_801C0F2C:
    {
        u32 teams = D_80108668;
        u32 total_matches = (u32)((s32)(teams * (teams - 1)) / 2);

        if (D_8010866A == total_matches) {
            D_8010A322 = 0x32;
        } else {
            D_8010A322++;
        }
        D_8010A320 = 0;
        return;
    }

L_801C0F9C:
    SPAD_2A2 = 0x41;
    func_801A8CD0();
    D_801D8DFC = 1;
    D_8010A320 = 0;
    D_8010A322++;
    return;

L_801C0FE4:
    {
        s32 res = func_801ACE2C((D_801D8DFB != 0) ? 2 : 0);
        if (res == 1) {
            /* Note: asm triggers break 7 if (D_80108668 >> 1) == 0 */
            if ((D_8010866A % (D_80108668 >> 1)) == 0) {
                D_8010A322 = 0x17;
            } else {
                D_8010A322++;
            }
        } else if (res == 2) {
            D_8010A322 = 0x23;
        } else if (res == 3) {
            SPAD_29F = 0;
            func_800171C4(0xFF);
        }
        return;
    }

L_801C10AC:
    if (func_8006F0DC(0x40, 0) == 0) {
        return;
    }
    func_801C2064();
    func_801C19EC(func_801C2A3C);
    D_8010A320 = 0;
    D_8010A322++;
    return;

L_801C10F8:
    if (func_8006F07C(0x10) != 0) {
        D_8010A322 = 4;
    }
    return;

L_801C1110:
    if (func_8006F0DC(0x40, 0) != 0) {
        D_8010A322++;
    }
    return;

L_801C1138:
    D_801D8DFC = 1;
    D_801D8DFB = 0;
    func_800171FC(1);
    D_8010A322 = 0x28;
    return;

L_801C115C:
    func_8018E538(0);
    SPAD_330 = 0;
    SPAD_29F = 0;
    D_801DA2F0 = 0;
    D_8010A322++;
    return;

L_801C1188:
    if (func_80019B64(0xCA) == 0) {
        return;
    }
    SPAD_330 = 1;
    D_8010A31E = 0xFF;
    func_801A8CD0();
    D_8010A322++;
    return;

L_801C11C8:
    func_800AB620(0x205);
    D_8010A322 = 1;
    return;

L_801C11D8:
    func_800171FC(4);
    D_8010A322 = 0x32;
}
