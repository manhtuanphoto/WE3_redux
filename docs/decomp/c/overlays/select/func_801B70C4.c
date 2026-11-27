/*
 * func_801B70C4 (overlay: select)
 *
 * Purpose:
 *   Secondary tournament/league/bracket state machine in the `select` overlay,
 *   dispatched on `D_8010A322` (u8, states 0..0x47 via `jtbl_8018B0FC`).
 *   Sets `D_801D8A48 = 1` (contrasting with `func_801B5A7C` which sets 0).
 *   Initializes tournament state (`D_801DA5D0..D_801DA5D6`, `D_8018B1F0` slot
 *   descriptors), simulates/advances matches (`func_801AA7C4(1)`, `func_801A9C54`),
 *   records winner/loser into `D_801D8803..D_801D8804` and `D_801D8A4C[6..8]`,
 *   and drives sub-menus (`func_801B7804`, `func_801AE294`, `func_801ACE2C`).
 *
 * Key Scratchpad (0x1F800000) offsets:
 *   +0x29F (u8, W) : commentary/loading presentation step (reset to 0 on exit)
 *   +0x2A2 (u8, W) : menu/overlay sub-flag (set to 0x41 on dialog enter)
 *   +0x2D1 (u8, R) : team 0 total goals
 *   +0x2D2 (u8, R) : team 1 total goals
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 */

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t  s32;

#define SCRATCHPAD_U8(off) (*(volatile u8 *)(0x1F800000U + (off)))

extern u8 D_800FF748[];

/* D_800FF748-relative globals */
#define D_8010865D (*(u8 *)(D_800FF748 + 0x8F15)) /* leg/fixture toggle flag */
#define D_80108668 (*(u8 *)(D_800FF748 + 0x8F20)) /* team/group count for D_8018B1F0 init */
#define D_8010866A (*(u8 *)(D_800FF748 + 0x8F22)) /* round/stage counter */
#define D_8010866B (*(u8 *)(D_800FF748 + 0x8F23)) /* match sub-phase */
#define D_8010866E_BASE (D_800FF748 + 0x8F26)     /* per-team +0x02: matches played count (stride 0x24) */
#define D_8010867F_BASE (D_800FF748 + 0x8F37)     /* per-team +0x13: round/stage value (stride 0x24) */
#define D_8010A2E8 (*(u8 *)(D_800FF748 + 0xABA0)) /* current match team 0 slot */
#define D_8010A2E9 (*(u8 *)(D_800FF748 + 0xABA1)) /* current match team 1 slot */
#define D_8010A2F0 (*(u8 *)(D_800FF748 + 0xABA8)) /* tournament champion slot */
#define D_8010A320 (*(u8 *)(D_800FF748 + 0xABD8)) /* sub-menu state */
#define D_8010A322 (*(u8 *)(D_800FF748 + 0xABDA)) /* primary state variable */

extern u8 **D_8018B1F0[]; /* 2D table of u8* descriptors (row stride 16 pointers = 0x40 bytes) */
extern u8   D_801D8803;   /* winner slot */
extern u8   D_801D8804;   /* loser slot */
extern u8   D_801D8A48;   /* mode flag (1 in func_801B70C4, 0 in func_801B5A7C) */
extern u8  *D_801D8A4C;   /* pointer to current stage/fixture descriptor */
extern u8   D_801D8B18;
extern u8   D_801D8DFB;
extern u8   D_801D8DFC;
extern u8   D_801DA2F0;
extern u8   D_801DA333;   /* team 0 goals snapshot */
extern u8   D_801DA334;   /* team 1 goals snapshot */
extern u16  D_801DA5D0;
extern u16  D_801DA5D2;
extern u16  D_801DA5D4;
extern u16  D_801DA5D6;
extern u8   D_801DA618;
extern u8   D_801DADB6;

extern void *jtbl_8018B0FC[];

extern void func_800171C4(s32 a0);
extern s32  func_8006F07C(s32 a0);
extern s32  func_8006F0DC(s32 a0, s32 a1);
extern s32  func_8018EA8C(s32 a0, s32 a1);
extern void func_801A9A08(void);
extern void func_801A9C54(void);
extern void func_801AA7C4(u8 arg0);
extern s32  func_801ACE2C(s32 arg0);
extern s32  func_801AE294(u8 arg0);
extern s32  func_801B7804(void);
extern void func_801B84D8(s32 a0);
extern void func_801B8708(u32 a0);
extern void func_801B8A20(s32 a0);
extern void func_801B8BB0(s32 a0, s32 a1);
extern void func_801B8CD4(void);
extern void func_801B8F38(void);
extern void func_801B9100(void);

void func_801B70C4(void)
{
    u8 state = D_8010A322;

    D_801D8A48 = 1;

    if (state >= 0x48) {
        return;
    }

    /* UNCLEAR: jtbl_8018B0FC (72 entries, states 0x00..0x47) is not included
     * in the compact asm. Below are all 17 jlabel targets plus 3 unlabeled
     * jump-table target blocks following stripped-nop `j .L801B77EC` exits.
     */
    (void)jtbl_8018B0FC[state];
    return;

L_801B7118:
    D_8010866A = 0;
    D_8010866B = 0;
    D_8010A2F0 = 0;
    D_801D8B18 = 1;
    D_801D8DFC = 0;
    D_801D8DFB = 0;
    D_801DA2F0 = 0;
    D_801DADB6 = 0;
    D_801DA5D0 = 0;
    D_801DA5D2 = 0;
    D_801DA5D4 = 1;
    D_801DA5D6 = 4;
    D_8010A2E9 = 0;
    D_8010A2E8 = 0;
    D_8010A322++;
    return;

L_801B71BC:
    func_801B84D8(1);
    D_8010A322++;
    return;

L_801B71D8:
    {
        s32 i;
        for (i = 0; i < (s32)D_80108668; i++) {
            u8 *entry = ((u8 **)((u8 *)D_8018B1F0 + (D_80108668 << 6)))[i];
            entry[6] = 3;
            entry[7] = (u8)i;
            entry[8] = 0xFF;
        }
    }
    func_801B84D8(0);
    D_8010A322 = 0x0A;
    return;

L_801B7260:
    func_801B8708(0);
    D_8010866B = 0;
    D_801DADB6 = 0;
    D_8010A322++;
    return;

L_801B7290:
    func_801B8CD4();
    if (D_801DA618 != 0) {
        D_8010A322++;
        return;
    }
    func_801B8A20(1);
    /* UNCLEAR: $a0 is not reloaded before this call after func_801B8A20(1) */
    func_801B8BB0(1, 2);
    func_801B8BB0(5, 3);
    func_801B84D8(0);
    return;

L_801B72F0:
    D_8010865D = 0;
    func_801A9A08();
    D_8010A322++;
    return;

L_801B7318:
    func_801B84D8(0);
    func_801AA7C4(1);
    D_8010A322++;
    return;

L_801B733C:
    func_801B8708(0);
    func_801B8CD4();
    func_801A9C54();
    D_8010866E_BASE[D_8010A2E8 * 0x24] += 1;
    D_8010866E_BASE[D_8010A2E9 * 0x24] += 1;
    D_8010867F_BASE[D_8010A2E8 * 0x24] = D_801D8A4C[5];
    D_8010867F_BASE[D_8010A2E9 * 0x24] = D_801D8A4C[5];
    D_8010A322++;
    return;

L_801B7458:
    {
        u8 g0 = SCRATCHPAD_U8(0x2D1);
        u8 g1 = SCRATCHPAD_U8(0x2D2);
        u8 winner, loser;

        D_801DA333 = g0;
        D_801DA334 = g1;
        if (g0 < g1) {
            winner = D_8010A2E9;
            loser  = D_8010A2E8;
        } else {
            winner = D_8010A2E8;
            loser  = D_8010A2E9;
        }
        D_801D8803 = winner;
        D_801D8804 = loser;
        D_801D8A4C[6] = 3;
        D_801D8A4C[7] = D_801D8803;
        D_801D8A4C[8] = D_801D8804;
        D_8010A322 = 0x14;
    }
    return;

L_801B751C:
    func_801B84D8(0);
    {
        u8 next_state = D_801D8A4C[4];
        if (next_state == 0xFF) {
            /* UNCLEAR: $a0 is not explicitly loaded before func_801B8A20 (0 remains from func_801B84D8(0)) */
            func_801B8A20(0);
            D_8010A2F0 = D_801D8803;
            func_801B8F38();
            D_801DA2F0 = 0;
            D_8010A322 = 0x1E;
        } else {
            D_8010A322 = next_state;
            D_8010866A++;
        }
    }
    return;

L_801B75B4:
    func_801B9100();
    D_801DADB6 = 1;
    D_8010A322++;
    return;

L_801B75DC:
    if (func_8006F07C(0x10) != 0) {
        D_8010A322++;
    }
    return;

L_801B7600:
    D_8010A322 += (u8)func_8018EA8C(1, 0);
    return;

L_801B7634:
    if (func_8006F0DC(0x40, 0) != 0) {
        D_8010A322 = 0x28;
    }
    return;

L_801B7650:
    {
        s32 res = func_801B7804();
        if (res == 1) {
            D_8010A322 = 0x1E;
        } else if (res == 2) {
            D_8010A322 = 0x32;
        }
    }
    return;

L_801B7690:
    {
        s32 res = func_801AE294(0);
        if (res == 1) {
            D_8010A322 = 0x28;
        } else if (res == 2) {
            D_8010A322 = 0x3C;
        }
    }
    return;

L_801B76D0:
    D_8010A320 = 0;
    D_8010A322++;
    return;

/* UNCLEAR: unlabeled jump-table target block at 0x801B76FC (after j .L801B77EC) */
L_801B76FC:
    {
        s32 res = func_801AE294(1);
        if (res == 1) {
            D_8010A322 = 0x32;
        } else if (res == 2) {
            D_8010A322 = 0x46;
        }
    }
    return;

/* UNCLEAR: unlabeled jump-table target block at 0x801B7748 (after j .L801B77EC) */
L_801B7748:
    D_801D8DFC = 0;
    D_801D8DFB = 1;
    SCRATCHPAD_U8(0x2A2) = 0x41;
    D_8010A320 = 0;
    D_8010A322++;
    return;

/* UNCLEAR: unlabeled jump-table target block at 0x801B7794 (after j .L801B77EC) */
L_801B7794:
    {
        s32 res = func_801ACE2C(3);
        if (res == 1) {
            D_8010A322 = 0x3C;
        } else if (res == 3) {
            func_801B8708(1);
            SCRATCHPAD_U8(0x29F) = 0;
            func_800171C4(0xFF);
        }
    }
    return;
}
