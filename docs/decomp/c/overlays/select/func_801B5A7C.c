/*
 * func_801B5A7C (overlay: select)
 *
 * Purpose:
 *   Main state machine for knockout/cup bracket progression and match/sim flow
 *   in the `select` overlay, dispatched on `D_8010A322` (u8, states 0..0xCD
 *   via `jtbl_8018ADC4`). Sets `D_801D8A48 = 0` (contrasting with `func_801B70C4`
 *   which sets `D_801D8A48 = 1`).
 *   Handles 1-leg vs 2-leg tie resolution (including away goals and extra-time/PK
 *   tiebreakers), simulated vs played match branches (`Scratchpad+0x2E2 == 4` /
 *   `D_80108667 & 0x20`), bracket winner/loser updates (`D_801D8A4C`, `D_801D8800`),
 *   sub-menu/confirmation flows (`func_801AE294`, `func_801ACE2C`), fade/palette
 *   ramps (`D_801D9320..D_801D9322`), and cup completion/unlock handling.
 *
 * Key Scratchpad (0x1F800000) offsets:
 *   +0x29F (u8, W)   : commentary/loading presentation step (reset to 0 on exit)
 *   +0x2A2 (u8, W)   : menu/overlay sub-flag (0 or 0x41)
 *   +0x2D1..0x2D2 (u8, R/W): team 0 / team 1 total goals
 *   +0x2D3..0x2D4 (u8, R)  : team 0 1H / 2H goals
 *   +0x2D5..0x2D6 (u8, R)  : team 0 extra-time 1H / 2H goals
 *   +0x2D7..0x2D8 (u8, R)  : team 1 1H / 2H goals
 *   +0x2D9..0x2DA (u8, R)  : team 1 extra-time 1H / 2H goals
 *   +0x2DB..0x2DC (u8, R)  : team 0 / team 1 PK shootout goals
 *   +0x2DD (u8, R/W)       : winning/selected team ID
 *   +0x2E2 (u8, R)         : match mode/flag (4 = simulated/CPU or skip branch)
 *   +0x330 (u8, W)         : screen/fade active flag
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
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;

typedef struct {
    s16 x;          /* 0x00 */
    s16 y;          /* 0x02 */
    s16 w;          /* 0x04 */
    s16 h;          /* 0x06 */
    u8  rgba[12];   /* 0x08..0x13 */
} MenuBoxDesc;

#define SCRATCHPAD_U8(off) (*(volatile u8 *)(0x1F800000U + (off)))

extern u8 D_800FF748[];

/* D_800FF748-relative globals */
#define D_8010865D (*(u8  *)(D_800FF748 + 0x8F15)) /* leg/fixture toggle flag (0 or 1) */
#define D_80108667 (*(u8  *)(D_800FF748 + 0x8F1F)) /* tournament flags (& 0x20: single-leg/mode, & 0x0F: cup type) */
#define D_80108669 (*(u8  *)(D_800FF748 + 0x8F21)) /* remaining active human/non-eliminated teams */
#define D_8010866A (*(u8  *)(D_800FF748 + 0x8F22)) /* current round/stage counter */
#define D_8010866B (*(u8  *)(D_800FF748 + 0x8F23)) /* match/leg sub-phase (0..3) */
#define D_8010866C_BASE (D_800FF748 + 0x8F24)      /* per-team record array (stride 0x24 bytes) */
#define D_8010866D_BASE (D_800FF748 + 0x8F25)      /* per-team +0x01: team ID */
#define D_8010866E_BASE (D_800FF748 + 0x8F26)      /* per-team +0x02: matches played count */
#define D_8010867F_BASE (D_800FF748 + 0x8F37)      /* per-team +0x13: round/stage value (D_801D8A4C[5]) */
#define D_8010A2C8 ((u8  *)(D_800FF748 + 0xAB80))  /* bracket slot -> team record index map */
#define D_8010A2E8 (*(u8  *)(D_800FF748 + 0xABA0)) /* current match team 0 slot */
#define D_8010A2E9 (*(u8  *)(D_800FF748 + 0xABA1)) /* current match team 1 slot */
#define D_8010A2EA (*(u8  *)(D_800FF748 + 0xABA2)) /* saved leg 1 team 0 total goals */
#define D_8010A2EB (*(u8  *)(D_800FF748 + 0xABA3)) /* saved leg 1 team 1 total goals */
#define D_8010A2EC (*(u8  *)(D_800FF748 + 0xABA4)) /* saved leg 1 team 0 1H goals */
#define D_8010A2ED (*(u8  *)(D_800FF748 + 0xABA5)) /* saved leg 1 team 0 2H goals */
#define D_8010A2EE (*(u8  *)(D_800FF748 + 0xABA6)) /* saved leg 1 team 1 1H goals */
#define D_8010A2EF (*(u8  *)(D_800FF748 + 0xABA7)) /* saved leg 1 team 1 2H goals */
#define D_8010A2F0 (*(u8  *)(D_800FF748 + 0xABA8)) /* tournament champion slot */
#define D_8010A31E (*(s16 *)(D_800FF748 + 0xABD6)) /* screen fade level */
#define D_8010A320 (*(u8  *)(D_800FF748 + 0xABD8)) /* sub-menu state */
#define D_8010A322 (*(u8  *)(D_800FF748 + 0xABDA)) /* primary state variable */
#define D_8010A32A (*(u8  *)(D_800FF748 + 0xABE2)) /* unlock/completion bitflag */

extern u8  D_801D8800[]; /* [0]=winner slot, [1]=loser slot */
extern u8  D_801D8802;
extern u8  D_801D8A48;   /* mode flag (0 in func_801B5A7C, 1 in func_801B70C4) */
extern u8 *D_801D8A4C;   /* pointer to current bracket/stage descriptor */
extern u8  D_801D8B18;
extern MenuBoxDesc D_801D8D78; /* 0x801D8D78..0x801D8D82 */
extern u8  D_801D8DFB;
extern u8  D_801D8DFC;
extern u8  D_801D9313;
extern u8  D_801D9320;
extern u8  D_801D9321;
extern u8  D_801D9322;
extern s32 D_801DA058;
extern u8  D_801DA2F0;
extern u8  D_801DA333;   /* team 0 aggregate/tiebreak score */
extern u8  D_801DA334;   /* team 1 aggregate/tiebreak score */
extern u8  D_801DA618;
extern u8  D_801DAD85;
extern u8  D_801DADB6;
extern u8  D_801DAE50;   /* confirmation dialog result (2 = confirm) */

extern void func_800171C4(s32 a0);
extern void func_800171FC(s32 a0);
extern s32  func_80019B64(s32 a0);
extern void func_8004FC54(s32 a0);
extern s32  func_8006F07C(s32 a0);
extern s32  func_8006F0DC(s32 a0, s32 a1);
extern void func_800AB620(s32 sfx_id);
extern void func_8018E538(s32 a0);
extern s32  func_8018EA8C(s32 a0, s32 a1);
extern s32  func_8018EAF4(s32 a0, s32 a1, s32 a2);
extern void func_801941E0(s32 a0, u32 a1, MenuBoxDesc *box, s32 a3, s32 s0, s32 s1);
extern void func_801A9A08(void);
extern void func_801A9C54(void);
extern void func_801AA7C4(u8 arg0);
extern void func_801AB290(s32 a0);
extern s32  func_801ACE2C(s32 arg0);
extern void func_801AD7EC(void);
extern s32  func_801AE294(u8 arg0);
extern void func_801B8450(s32 a0);
extern void func_801B84D8(s32 a0);
extern void func_801B8708(u32 a0);
extern void func_801B8A20(s32 a0);
extern void func_801B8BB0(s32 a0, s32 a1);
extern void func_801B8CD4(void);
extern s32  func_801B8EB8(void);
extern void func_801B8F38(void);
extern void func_801B9100(void);
extern s32  func_801B9144(s32 a0);

void func_801B5A7C(void)
{
    u8 state = D_8010A322;

    D_801D8A48 = 0;

    if (state >= 0xCE) {
        return;
    }

    switch (state) {
    case 0x00:
        func_801B8708(0);
        func_8004FC54(0);
        SCRATCHPAD_U8(0x2A2) = 0;
        D_8010A322 = 0x0A;
        break;

    case 0x0A:
        D_8010866B = 0;
        D_801DADB6 = 0;
        D_801D8B18 = 0;
        D_8010A322++;
        break;

    case 0x0B:
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
        break;

    case 0x0C:
        D_8010865D = 0;
        func_801A9A08();
        D_8010A2EF = 0;
        D_8010A2ED = 0;
        D_8010A2EE = 0;
        D_8010A2EC = 0;
        D_8010A2EB = 0;
        D_8010A2EA = 0;
        D_8010A322++;
        break;

    case 0x0D:
        if (SCRATCHPAD_U8(0x2E2) == 4) {
            D_8010A322 = 0x28;
        } else {
            D_8010A322 = 0x14;
        }
        break;

    case 0x14:
        D_801DA2F0 = 1;
        func_801B9100();
        D_8010A322++;
        break;

    case 0x15:
        if (func_8006F07C(0x10) != 0) {
            D_8010A322++;
        }
        break;

    case 0x17:
        if (func_8006F0DC(0x40, 1) != 0) {
            D_8010A322 = 0x1E;
        }
        break;

    case 0x1E:
        if ((D_80108667 & 0x20) != 0) {
            D_8010A322 = 0x28;
            return;
        }
        D_801D8802 = (u8)func_801B9144(0);
        D_8010A322++;
        break;

    case 0x1F:
        if (func_8006F07C(0x10) != 0) {
            D_8010A322++;
        }
        break;

    case 0x20:
        func_801B8450(0);
        D_8010A322 += (u8)func_8018EAF4(1, 1, 0);
        break;

    case 0x21:
        func_801B8450(-1);
        if (func_8006F0DC(0x40, 1) != 0) {
            if (D_801DAE50 == 2) {
                D_8010A322 = 0x28;
            } else {
                D_8010A322 = 0x14;
            }
        }
        break;

    case 0x28:
        func_801B84D8(0);
        if (SCRATCHPAD_U8(0x2E2) == 4) {
            func_801AA7C4((D_80108667 & 0x20) ? 1 : 0);
            D_8010A322 = 0x32;
        } else if ((D_80108667 & 0x20) != 0) {
            func_801AB290(1);
        } else {
            func_801AB290(0);
        }
        break;

    case 0x32:
        D_8010866B = 1;
        func_801B8708(0);
        func_801B8CD4();
        D_8010865D = 0;
        func_801A9A08();

        /* Save 1st-leg scoreline & half goals from Scratchpad */
        D_8010A2EA = SCRATCHPAD_U8(0x2D1);
        D_8010A2EB = SCRATCHPAD_U8(0x2D2);
        D_8010A2EC = SCRATCHPAD_U8(0x2D3);
        D_8010A2EE = SCRATCHPAD_U8(0x2D7);
        D_8010A2ED = SCRATCHPAD_U8(0x2D4);
        D_8010A2EF = SCRATCHPAD_U8(0x2D8);

        func_801A9C54();
        D_8010866E_BASE[D_8010A2E8 * 0x24] += 1;
        D_8010866E_BASE[D_8010A2E9 * 0x24] += 1;
        D_8010867F_BASE[D_8010A2E8 * 0x24] = D_801D8A4C[5];
        D_8010867F_BASE[D_8010A2E9 * 0x24] = D_801D8A4C[5];
        D_8010A322++;
        break;

    case 0x33:
        func_801B84D8(0);
        if ((D_80108667 & 0x20) != 0) {
            u8 t0_score = (u8)(SCRATCHPAD_U8(0x2D1) + SCRATCHPAD_U8(0x2DB));
            u8 t1_score = (u8)(SCRATCHPAD_U8(0x2D2) + SCRATCHPAD_U8(0x2DC));
            u8 winner, loser;
            u8 loser_idx;

            D_801DA333 = t0_score;
            D_801DA334 = t1_score;
            if (t0_score < t1_score) {
                winner = D_8010A2E9;
                loser  = D_8010A2E8;
            } else {
                winner = D_8010A2E8;
                loser  = D_8010A2E9;
            }
            D_801D8800[0] = winner;
            D_801D8800[1] = loser;
            D_801D8A4C[6] = 3;
            D_801D8A4C[7] = D_801D8800[0];
            D_801D8A4C[8] = D_801D8800[1];

            loser_idx = D_8010A2C8[D_801D8800[1]];
            if ((D_8010866C_BASE[loser_idx * 0x24] & 0xC0) == 0) {
                D_80108669--;
            }
        }
        D_8010A322++;
        break;

    case 0x34:
        if (SCRATCHPAD_U8(0x2E2) == 4) {
            if ((D_80108667 & 0x20) != 0) {
                D_8010A322 = 0xA0;
            } else {
                D_8010A322 = 0x5A;
            }
        } else {
            D_8010A322++;
        }
        break;

    case 0x35:
        if ((D_80108667 & 0x20) != 0) {
            D_8010A322 = 0x82;
        } else {
            D_8010A322 = 0x3C;
        }
        break;

    case 0x3C:
        D_801D8802 = (u8)func_801B9144(1);
        D_8010A322++;
        break;

    case 0x3D:
        if (func_8006F07C(0x10) != 0) {
            D_8010A322++;
        }
        break;

    case 0x3E:
        func_801B8450(1);
        D_8010A322 += (u8)func_8018EA8C(1, 0);
        break;

    case 0x3F:
        func_801B8450(-1);
        if (func_8006F0DC(0x40, 1) != 0) {
            D_8010A322 = 0x46;
        }
        break;

    case 0x46:
        D_8010866B = 2;
        func_801B8CD4();
        D_8010865D = 1;
        func_801A9A08();
        D_801D8802 = (u8)func_801B9144(2);
        D_8010A322 = 0x50;
        break;

    case 0x50:
        if (func_8006F07C(0x10) != 0) {
            D_8010A322++;
        }
        break;

    case 0x51:
        func_801B8450(2);
        D_8010A322 += (u8)func_8018EA8C(1, 0);
        break;

    case 0x52:
        func_801B8450(-1);
        if (func_8006F0DC(0x40, 1) != 0) {
            D_8010A322 = 0x5A;
        }
        break;

    case 0x5A:
        func_801B84D8(0);
        if (SCRATCHPAD_U8(0x2E2) == 4) {
            func_801AA7C4(0);
            D_8010A322 = 0x64;
        } else {
            func_801AB290(1);
        }
        break;

    case 0x64:
        D_8010866B = 3;
        func_801B8708(0);
        func_801B8CD4();
        D_8010865D = 1;
        func_801A9A08();
        func_801A9C54();
        D_8010866E_BASE[D_8010A2E8 * 0x24] += 1;
        D_8010866E_BASE[D_8010A2E9 * 0x24] += 1;
        D_8010867F_BASE[D_8010A2E8 * 0x24] = D_801D8A4C[5];
        D_8010867F_BASE[D_8010A2E9 * 0x24] = D_801D8A4C[5];
        D_8010A322++;
        break;

    case 0x65:
        if (SCRATCHPAD_U8(0x2E2) != 4) {
            D_8010A322++;
            return;
        }
        if (func_801B8EB8() != 0) {
            D_801DA333 = SCRATCHPAD_U8(0x2D1);
            D_801DA334 = SCRATCHPAD_U8(0x2D2);
            func_801AA7C4(2);
            SCRATCHPAD_U8(0x2D1) = (u8)(SCRATCHPAD_U8(0x2D1) + D_801DA333);
            SCRATCHPAD_U8(0x2D2) = (u8)(SCRATCHPAD_U8(0x2D2) + D_801DA334);
            D_801DA333 = SCRATCHPAD_U8(0x2D1);
            D_801DA334 = SCRATCHPAD_U8(0x2D2);
        }
        {
            u8 winner, loser;
            if (D_801DA333 < D_801DA334) {
                winner = D_8010A2E9;
                loser  = D_8010A2E8;
            } else {
                winner = D_8010A2E8;
                loser  = D_8010A2E9;
            }
            D_801D8800[0] = winner;
            D_801D8800[1] = loser;
            D_801D8A4C[6] = 3;
            D_801D8A4C[7] = D_801D8800[0];
            D_801D8A4C[8] = D_801D8800[1];
            D_8010A2E8 = D_801D8800[0];
            D_8010A2E9 = D_801D8800[1];
        }
        D_8010A322 = 0xA0;
        break;

    case 0x66:
        /* Aggregate two-leg score calculation + away-goals/ET/PK tiebreakers */
        D_801DA333 = (u8)(D_8010A2EA + SCRATCHPAD_U8(0x2D3) + SCRATCHPAD_U8(0x2D4));
        D_801DA334 = (u8)(D_8010A2EB + SCRATCHPAD_U8(0x2D7) + SCRATCHPAD_U8(0x2D8));

        if (D_801DA333 == D_801DA334) {
            u8 t0_leg2 = (u8)(SCRATCHPAD_U8(0x2D3) + SCRATCHPAD_U8(0x2D4));
            u8 t1_leg2 = (u8)(SCRATCHPAD_U8(0x2D7) + SCRATCHPAD_U8(0x2D8));
            u8 agg0, agg1;

            if (D_8010865D != 0) {
                agg0 = (u8)(D_8010A2EA + (t0_leg2 << 1));
                agg1 = (u8)((D_8010A2EB << 1) + t1_leg2);
            } else {
                agg0 = (u8)((D_8010A2EA << 1) + t0_leg2);
                agg1 = (u8)(D_8010A2EB + (t1_leg2 << 1));
            }
            D_801DA333 = agg0;
            D_801DA334 = agg1;

            if (agg0 == agg1) {
                D_801DA333 = (u8)(agg0 + SCRATCHPAD_U8(0x2D5) + SCRATCHPAD_U8(0x2D6) + SCRATCHPAD_U8(0x2DB));
                D_801DA334 = (u8)(agg1 + SCRATCHPAD_U8(0x2D9) + SCRATCHPAD_U8(0x2DA) + SCRATCHPAD_U8(0x2DC));
            }
        }
        {
            u8 winner, loser;
            u8 loser_idx;
            if (D_801DA333 < D_801DA334) {
                winner = D_8010A2E9;
                loser  = D_8010A2E8;
            } else {
                winner = D_8010A2E8;
                loser  = D_8010A2E9;
            }
            D_801D8800[0] = winner;
            D_801D8800[1] = loser;
            D_801D8A4C[6] = 3;
            D_801D8A4C[7] = D_801D8800[0];
            D_801D8A4C[8] = D_801D8800[1];
            D_8010A2E8 = D_801D8800[0];
            D_8010A2E9 = D_801D8800[1];

            loser_idx = D_8010A2C8[D_801D8800[1]];
            if ((D_8010866C_BASE[loser_idx * 0x24] & 0xC0) == 0) {
                D_80108669--;
            }
        }
        D_8010A322 = 0x6E;
        break;

    case 0x6E:
        D_801D8802 = (u8)func_801B9144(3);
        D_8010A322++;
        break;

    case 0x6F:
        if (func_8006F07C(0x10) != 0) {
            D_8010A322++;
        }
        break;

    case 0x70:
        func_801B8450(3);
        D_8010A322 += (u8)func_8018EA8C(1, 0);
        break;

    case 0x71:
        func_801B8450(-1);
        if (func_8006F0DC(0x40, 1) != 0) {
            D_8010A322 = 0x78;
        }
        break;

    case 0x78:
        if (func_801B8EB8() == 0) {
            D_8010A322 = 0x82;
            return;
        }
        D_801D8802 = (u8)func_801B9144(4);
        D_8010A322++;
        break;

    case 0x79:
        if (func_8006F07C(0x10) != 0) {
            D_8010A322++;
        }
        break;

    case 0x7A:
        D_8010A322 += (u8)func_8018EAF4(1, 1, 0);
        break;

    case 0x7B:
        if (func_8006F0DC(0x40, 1) != 0) {
            if (D_801DAE50 == 2) {
                D_8010A322 = 0x82;
            } else {
                D_8010A322 = 0x6E;
            }
        }
        break;

    case 0x82:
        func_801B9100();
        D_801DADB6 = 1;
        D_8010A322++;
        break;

    case 0x83:
        if (func_8006F07C(0x10) != 0) {
            D_8010A322++;
        }
        break;

    case 0x84:
        if (D_801D8B18 == 1 || D_801D8DFB == 1 || (D_80108667 & 0x20) != 0) {
            D_8010A322 += (u8)func_8018EA8C(1, 0);
        } else {
            D_8010A322 += (u8)func_8018EAF4(1, 2, 0);
        }
        break;

    case 0x16:
        D_8010A322 += (u8)func_8018EA8C(1, 0);
        break;

    case 0x85:
        if (func_8006F0DC(0x40, 1) != 0) {
            D_8010A322 = 0x8C;
        }
        break;

    case 0x86:
        if (func_8006F0DC(0x40, 1) != 0) {
            if (func_801B8EB8() == 0) {
                D_8010A322 = 0x6E;
                return;
            }
            func_801B9144(4);
            D_8010A322 = 0x79;
        }
        break;

    case 0x8C:
        D_8010A320 = 0;
        D_8010A322++;
        break;

    case 0x8D:
        {
            s32 res = func_801AE294(0);
            if (res == 1) {
                D_8010A322 = 0x82;
            } else if (res == 2) {
                D_8010A322 = 0x96;
            }
        }
        break;

    case 0x96:
        D_8010A320 = 0;
        D_8010A322++;
        break;

    case 0x97:
        {
            s32 res = func_801AE294(1);
            if (res == 1) {
                D_8010A322 = 0x8C;
            } else if (res == 2) {
                D_8010A322 = 0xA0;
            }
        }
        break;

    case 0xA0:
        if (D_80108669 == 0 || D_801D8B18 == 1) {
            D_8010A322 = 0xC8;
            return;
        }
        if (D_801D8A4C[4] == 0xFF && D_801D8DFB == 0) {
            D_8010A322 = 0xBE;
            return;
        }
        func_801B84D8(0);
        D_8010A322++;
        break;

    case 0xA1:
        if (SCRATCHPAD_U8(0x2E2) == 4) {
            D_8010A322 = 0xAC;
        } else {
            D_8010A322 = 0xAA;
        }
        break;

    case 0xAA:
        D_801D8DFC = 1;
        D_8010A320 = 0;
        D_8010A322++;
        break;

    case 0xAB:
        {
            s32 arg = (D_801D8DFB != 0) ? 2 : 0;
            s32 res = func_801ACE2C(arg);

            if (res == 1) {
                D_8010A322 = 0x96;
            } else if (res == 2) {
                D_8010A322++;
            } else if (res == 3) {
                func_801B8708(1);
                SCRATCHPAD_U8(0x29F) = 0;
                func_800171C4(0xFF);
            }
        }
        break;

    case 0xAC:
        if (func_8006F0DC(0x40, 0) != 0) {
            D_801D8DFC = 1;
            D_801D8DFB = 0;
            D_8010A322 = 0x0A;
            D_8010866A++;
        }
        break;

    case 0xB4:
        func_8018E538(0);
        func_801B8708(0);
        SCRATCHPAD_U8(0x330) = 0;
        D_801DADB6 = 1;
        SCRATCHPAD_U8(0x29F) = 0;
        D_8010A322++;
        break;

    case 0xB5:
        if (func_80019B64(0xCA) != 0) {
            SCRATCHPAD_U8(0x330) = 1;
            D_8010A31E = 0xFF;
            D_8010A322++;
        }
        break;

    case 0xB6:
        func_800AB620(0x205);
        D_8010A322 = 0x82;
        break;

    case 0xBE:
        if (func_8006F0DC(0x10, 1) != 0) {
            u8 team_idx;

            D_801DA2F0 = 0;
            D_801DAD85 = 1;
            team_idx = D_8010A2C8[D_801D8800[0]];
            SCRATCHPAD_U8(0x2A2) = 0;
            SCRATCHPAD_U8(0x2DD) = D_8010866D_BASE[team_idx * 0x24];
            if ((D_80108667 & 0x0F) == 0 && SCRATCHPAD_U8(0x2DD) == 2) {
                D_8010A32A |= 1;
            }
            func_800AB620(7);
            D_8010A320 = 0;
            D_8010A322++;
        }
        break;

    case 0xBF:
        func_801AD7EC();
        break;

    case 0xC8:
        D_801D8DFB = 1;
        D_801D8DFC = 0;
        SCRATCHPAD_U8(0x2A2) = 0x41;
        D_801D8D78.x = 0;
        D_801D8D78.w = 0x190;
        D_801D8D78.h = 0xF0;
        D_801D8D78.y = 0;
        D_801D8D78.rgba[0] = 0;
        D_801D8D78.rgba[1] = 0;
        D_801D8D78.rgba[2] = 0;
        func_801941E0(4, 0x60000000U, &D_801D8D78, 1, 1, 1);
        D_801DA058 = 0;
        D_8010A322++;
        break;

    case 0xC9:
        if (D_801DA058 < 0x10) {
            D_801DA058++;
            D_801D9320 += 2;
            D_801D9321 += 2;
            D_801D9322 += 2;
        } else {
            D_801D9320 = 0x20;
            D_801D9321 = 0x20;
            D_801D9322 = 0x20;
            D_8010A320 = 0;
            D_8010A322++;
        }
        break;

    case 0xCA:
        D_801D8B18 = 1;
        D_801D8D78.x = 0;
        D_801D8D78.w = 0x190;
        D_801D8D78.h = 0xF0;
        D_801D8D78.rgba[0] = 0x20;
        D_801D8D78.rgba[1] = 0x20;
        D_801D8D78.rgba[2] = 0x20;
        D_801D8D78.y = 0;
        func_801941E0(4, 0x60000000U, &D_801D8D78, 1, 1, 1);
        SCRATCHPAD_U8(0x2A2) = 0x41;
        D_8010A322++;
        break;

    case 0xCB:
        {
            s32 arg = ((D_80108667 & 0x0F) != 0) ? 3 : 1;
            s32 res = func_801ACE2C(arg);

            if (res == 1) {
                D_801DA058 = 0;
                D_8010A322++;
            } else if (res == 2) {
                func_800171FC(2);
                if (D_801D8A4C[4] == 0xFF) {
                    D_801D8800[0] = D_801D8A4C[7];
                    D_8010A2F0 = D_801D8A4C[7];
                    func_801B8F38();
                    D_801DA2F0 = 0;
                    D_8010A322 = 0x1E;
                } else {
                    func_801B84D8(0);
                    D_8010A322 = 0x0A;
                }
            } else if (res == 3) {
                SCRATCHPAD_U8(0x29F) = 0;
                func_801B8708(1);
                func_800171C4(0xFF);
            }
        }
        break;

    case 0xCC:
        if (D_801DA058 < 0x10) {
            D_801DA058++;
            D_801D9320 -= 2;
            D_801D9321 -= 2;
            D_801D9322 -= 2;
        } else {
            D_801D9313 = 0;
            D_8010A322++;
        }
        break;

    case 0xCD:
        D_8010A322 = 0x96;
        break;
    default:
        break;
    }
}
