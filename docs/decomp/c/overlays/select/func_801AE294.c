/*
 * func_801AE294 (overlay: select)
 *
 * Purpose:
 *   Top-level state machine for the team/cup/league selection browser in the
 *   select overlay, dispatching on D_8010A320 (states 0..0x2A via jtbl_8018A474)
 *   and sub-state D_801DAD84 (0..5 via jtbl_8018A524 / 53C / 554 / 56C / 584).
 *   Manages list initialization, scrolling (D_801D86EA / D_801D86F0), cursor
 *   movement, confirm/cancel transitions, and PS1 Scratchpad flags (0x1F8002A2,
 *   0x1F800330).
 *
 * Arguments:
 *   arg0 (u8 in $a0): mode/flag stored into D_801D86F2 (controls exit state 0x28 vs 0x29)
 *
 * Return value:
 *   0 while active/idle; 1, 4, or D_801DAE50 on exit states
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef signed short   s16;
typedef unsigned int   u32;
typedef signed int     s32;

/* PS1 Scratchpad RAM (0x1F800000..0x1F8003FF) */
#define SPAD_2A2 (*(volatile u8 *)0x1F8002A2)
#define SPAD_330 (*(volatile u8 *)0x1F800330)

extern u8  D_800FF748[];    /* base symbol for select/work RAM offsets */
extern s16 D_8010A5A4;      /* last pad input */
extern u8  D_801D86E0;      /* selected item value 0 */
extern u8  D_801D86E1;      /* selected item value 1 */
extern s16 D_801D86E8;
extern s16 D_801D86EA;      /* scroll top index */
extern s16 D_801D86EC;
extern s16 D_801D86EE;      /* visible page size (9) */
extern u16 D_801D86F0;      /* cursor row within visible page */
extern u8  D_801D86F2;      /* arg0 mode flag */
extern u8  D_801D86F3;      /* total list item count */
extern u8  D_801D8A51;      /* saved previous D_801DAD84 sub-state */
extern s16 D_801D8A82;      /* cursor/preview X position */
extern s16 D_801D8A84;      /* cursor/preview Y position */
extern s16 D_801D8A86;
extern s16 D_801D8A88;
extern u8  D_801D8A8A;
extern u8  D_801D8A8B;
extern u8  D_801D8A8C;
extern u8  D_801D8A8D;
extern u8  D_801D8A8E;
extern u8  D_801D8A8F;
extern u8  D_801D8B20[];    /* list entry table (stride 10 bytes, field +0) */
extern u8  D_801D8B21[];    /* list entry table (stride 10 bytes, field +1) */
extern s32 D_801DA058;
extern u8  D_801DA2F0;      /* active/ready flag */
extern u8  D_801DAD84;      /* UI sub-state (0..5) */
extern u8  D_801DAE50;      /* result/action code (0=none, 1=cancel, 2=confirm) */

#define D_8010564C (*(u8  *)(D_800FF748 + 0x5F04))
#define D_8010A31E (*(u16 *)(D_800FF748 + 0xABD6))
#define D_8010A320 (*(u8  *)(D_800FF748 + 0xABD8))

extern void *jtbl_8018A474[];
extern void *jtbl_8018A524[];
extern void *jtbl_8018A53C[];
extern void *jtbl_8018A554[];
extern void *jtbl_8018A56C[];
extern void *jtbl_8018A584[];

extern u8  *func_8001D004(s32 id);
extern s32  func_8006F0DC(s32 a0, s32 a1);
extern void func_8006FCFC(s32 a0);
extern void func_800AB620(s32 sfx_id);
extern s32  func_8018E538(s32 a0);
extern void func_801A8CD0(void);
extern void func_801AB910(s32 a0, u32 a1);
extern void func_801AC0B4(s32 a0, u32 a1, u8 a2);
extern void func_801AEBF8(void);
extern void func_801AECB4(void);
extern void func_801AF198(void);
extern void func_801AF480(u8 scroll_pos);
extern void func_801AF938(s32 a0);
extern void func_801AFECC(void);
extern s32  func_801AFF1C(void);
extern s32  func_801B015C(s32 a0);
extern s32  func_801B04B4(s32 a0);
extern u8   func_801B4FA8(u8 entry_id);

s32 func_801AE294(u8 arg0)
{
    u8 *obj;
    u8 state;
    s32 pad;

    D_801D86F2 = arg0;

    if (D_801DAD84 == 4 && D_801DA2F0 == 1) {
        obj = func_8001D004(0x30AC);
        obj[0xA] = 0x36;
        func_8006FCFC(0x36);
    } else {
        obj = func_8001D004(0x30AC);
        obj[0xA] = 0x5B;
    }

    state = D_8010A320;
    if (state >= 0x2B) {
        return 0;
    }

    /* UNCLEAR: jtbl_8018A474 (43 entries) is not present in the compact asm.
     * Below are all branch targets (jlabels) referenced by the outer jump table
     * and the inner D_801DAD84 jump tables (jtbl_8018A524/53C/554/56C/584).
     */
    (void)jtbl_8018A474[state];
    return 0;

L_801AE340:
    {
        u8 flags = D_801D8A8E;
        D_801D8A8A = 0x5F;
        D_801D8A8B = 0xFF;
        D_801D8A8D = 0x60;
        D_801DAD84 = 4;
        D_801DA2F0 = 0;
        D_801D8A82 = 0;
        D_801D8A84 = 0;
        D_801D8A86 = 0;
        D_801D8A88 = 0;
        D_801D8A8F = 0;
        D_801D8A8C = 0;
        D_801D8A51 = 1;
        D_801D86F0 = 0;
        D_801DA058 = 0;
        D_801D86F3 = 0;
        D_801D8A8E = (u8)((flags & 0x61) | 0x21);
        func_8018E538(0);
        SPAD_2A2 = 0x41;
        func_801A8CD0();
        D_8010A320++;
        return 0;
    }

L_801AE40C:
    D_801D86EC = 1;
    D_801D86E8 = 0;
    D_801D86EA = 0;
    D_801D86EE = 9;
    func_801AECB4();
    func_801AF198();
    func_801AF480((u8)D_801D86EA);
    func_801AFECC();
    if (D_801D86F3 == 0) {
        D_801DAD84 = 4;
        D_801D8A51 = 4;
    }
    D_8010A320++;
    return 0;

L_801AE494:
    SPAD_330 = 0;
    D_8010A31E = 0;
    D_8010A320++;
    return 0;

L_801AE4C8:
    if (func_801AFF1C() == 0) {
        return 0;
    }
    D_801D8A8F = 1;
    D_8010A320 = 0xA;
    D_801DA2F0 = 1;
    return 0;

L_801AE500:
    D_801DAE50 = 0;
    func_801AEBF8();
    pad = func_801B04B4(0);
    D_8010A5A4 = (s16)pad;
    pad &= 0xFFFF;

    if (pad == 0x1000) {
        /* Up */
        func_800AB620(0x50D);
        if (D_801DAD84 >= 6) {
            return 0;
        }
        /* UNCLEAR: dispatches via jtbl_8018A524[D_801DAD84] to one of:
         *   .L801AE568: D_801D8A84 -= 0x10;
         *   .L801AE58C: D_801DAD84 = 4;
         *   .L801AE5A0: cursor/scroll up (below)
         */
        (void)jtbl_8018A524[D_801DAD84];
        if (D_801D86F0 != 0) {
            D_801D86F0--;
            return 0;
        }
        if (D_801D86EA > 0) {
            D_801D86EA--;
            func_801AF480((u8)D_801D86EA);
            func_801AF938(0);
            return 0;
        }
        D_801D8A51 = D_801DAD84;
        D_801DAD84 = 4;
        return 0;
    } else if (pad == 0x4000) {
        /* Down */
        func_800AB620(0x50D);
        if (D_801D86F3 == 0) {
            D_801D8A84 += 0x10;
            return 0;
        }
        if (D_801DAD84 >= 6) {
            return 0;
        }
        /* UNCLEAR: dispatches via jtbl_8018A53C[D_801DAD84] to one of:
         *   .L801AE640: func_800AB620(0x50D); D_801D8A84 += 0x10;
         *   .L801AE650: D_801DAD84 = D_801D8A51;
         *   .L801AE668: cursor/scroll down (below)
         *   .L801AE6EC: D_801D8A84 += 0x10;
         */
        (void)jtbl_8018A53C[D_801DAD84];
        if (D_801D86F0 < 8) {
            if ((s32)D_801D86F0 < (s32)D_801D86F3 - 1) {
                D_801D86F0++;
            } else {
                D_801D8A84 += 0x10;
            }
        } else if (D_801D86EA + D_801D86EE < D_801D86F3) {
            D_801D86EA++;
            func_801AF480((u8)D_801D86EA);
            func_801AF938(0);
        } else {
            D_801D8A84 += 0x10;
        }
        return 0;
    } else if (pad == 0x8000) {
        /* Left */
        func_800AB620(0x50D);
        if (D_801D86F3 == 0 || D_801DAD84 >= 6) {
            return 0;
        }
        /* UNCLEAR: dispatches via jtbl_8018A554[D_801DAD84] to one of:
         *   .L801AE76C: D_801D8A82 -= 0x10;
         *   .L801AE788: D_801DAD84 = (D_801D8A51 == 2) ? 1 : D_801D8A51;
         */
        (void)jtbl_8018A554[D_801DAD84];
        return 0;
    } else if (pad == 0x2000) {
        /* Right */
        func_800AB620(0x50D);
        if (D_801DAD84 >= 6) {
            return 0;
        }
        /* UNCLEAR: dispatches via jtbl_8018A56C[D_801DAD84] to one of:
         *   .L801AE7F0: if (D_801D86F3 != 0) D_801DAD84 = 2; else D_801D8A82 += 0x10;
         *   .L801AE804: D_801D8A82 += 0x10;
         *   .L801AE830: D_801DAD84 = 1;
         *   .L801AE844: D_801DAD84 = (D_801D86F0 < 5) ? 2 : 3;
         *   .L801AE870: D_801DAD84 = 3;
         */
        (void)jtbl_8018A56C[D_801DAD84];
        return 0;
    } else if (pad == 0x20) {
        /* Circle / Confirm */
        if (D_801DAD84 >= 6) {
            return 0;
        }
        /* UNCLEAR: dispatches via jtbl_8018A584[D_801DAD84] to one of:
         *   .L801AE8BC: func_800AB620(0x528); D_801DAD84 = D_801D8A51; func_801AB910(-1, 0);
         *   .L801AE8CC: func_800AB620(0x528); preview/action via func_801AC0B4 (below)
         *   .L801AE964: func_800AB620(0x528); select via func_801AB910 (below)
         *   .L801AE9C8: page/scroll up 1 if D_801D86EA != 0
         *   .L801AE9F4: page/scroll down 1 if D_801D86EA + D_801D86EE < D_801D86F3
         *   .L801AEA54: func_800AB620(0x528); D_801DAE50 = 2; D_801DA2F0 = 0; D_8010A320 = 0x14;
         */
        (void)jtbl_8018A584[D_801DAD84];
        {
            u32 row = (u32)(D_801D86EA + D_801D86F0);
            u8 val = func_801B4FA8(D_801D8B20[row * 10]);
            D_801D86E0 = val;
            D_801D86E1 = D_801D8B21[row * 10];
            func_801AC0B4(0, val, D_801D86E1);
            D_801D8A51 = D_801DAD84;
            D_801DAD84 = 5;
        }
        return 0;
    } else if (pad == 0x40) {
        /* Cross / Cancel */
        func_800AB620(0x529);
        if (D_801DAD84 == 4) {
            D_801DAE50 = 1;
            D_801DA2F0 = 0;
            D_8010A320 = 0x14;
            D_801D8A51 = 4;
            D_801DAD84 = 4;
        } else if (D_801DAD84 == 5) {
            D_801DAD84 = D_801D8A51;
            func_801AB910(-1, 0);
        } else {
            D_801D8A51 = D_801DAD84;
            D_801DAD84 = 4;
        }
        return 0;
    }
    return 0;

L_801AEB1C:
    D_801D8A8F = 0;
    D_8010564C = 0;
    if (D_801DAE50 == 2) {
        D_8010A320 = 0x1E;
    } else if (D_801D86F2 == 0) {
        D_8010A320 = 0x28;
    } else {
        D_8010A320 = 0x29;
    }
    return 0;

L_801AEB90:
    if (func_801B015C(0) == 0) { /* UNCLEAR: $a0 not set before jal func_801B015C */
        return 0;
    }
    return (s32)D_801DAE50;

L_801AEBB0:
    /* UNCLEAR: asm sets $a0 = 0x10 in delay slot of jal func_801B015C,
     * then calls func_8006F0DC(?, 0) without reloading $a0.
     */
    func_801B015C(0x10);
    if (func_8006F0DC(0x10, 0) == 0) {
        return 0;
    }
    return 1;

L_801AEBD4:
    return 4;
}
