/*
 * func_801ACE2C (overlay: select)
 *
 * Purpose:
 *   State machine handler for a sub-menu/confirmation dialog in the select
 *   overlay, dispatching on D_8010A320 (states 0..0x34 via jtbl_8018A370).
 *   Handles menu box/cursor rendering, pad input navigation (Up/Down/Circle/Cross),
 *   sub-menu branching, and fade/transition synchronization.
 *
 * Arguments:
 *   arg0 (s32 in $a0): menu layout/option table index (used with D_801D276C..D_801D2788)
 *
 * Return value:
 *   0 in normal/pending states, 1/2/3 on terminal/exit branch labels
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef signed short   s16;
typedef unsigned int   u32;
typedef signed int     s32;

typedef struct {
    s16 x;          /* 0x00 */
    s16 y;          /* 0x02 */
    s16 w;          /* 0x04 */
    s16 h;          /* 0x06 */
    u8  rgba[12];   /* 0x08..0x13 */
} MenuBoxDesc;

extern u8  D_800FF748[];    /* base symbol for select/work RAM offsets */
extern u16 D_800AD638;      /* current menu cursor index */
extern u8  D_8010A320;      /* main sub-menu state machine index (D_800FF748 + 0xABD8) */
extern s16 D_8010A5A4;      /* last pad button mask */
extern u32 D_8011D5F4[];    /* menu label/sprite pointer table */
extern u32 D_8011D73C;      /* confirmation prompt pointer */
extern s16 D_801D276C[];    /* menu layout table (stride 6 bytes) */
extern s16 D_801D276E[];    /* cursor base X (stride 6 bytes) */
extern s16 D_801D2770[];    /* cursor step / Y (stride 6 bytes) */
extern u8  D_801D2784[];    /* label ID table per arg0 */
extern u8  D_801D2788[];    /* option count table per arg0 */
extern u16 D_801D273C[];    /* animation frame width table (stride 4 bytes) */
extern u16 D_801D273E[];    /* animation frame height table (stride 4 bytes) */
extern u8  D_801D269C;      /* confirmation selection snapshot */
extern u8  D_801D269D;      /* confirmation active flag */
extern u8  D_801D86BE;      /* dialog box open/close animation step (0..11) */
extern u8  D_801D86BF;      /* yes/no confirmation cursor (D_801D86BE + 1) */
extern MenuBoxDesc D_801D8D78; /* primitive/box descriptor */
extern u8  D_801D8FA0;
extern u8  D_801D8FB8;
extern u8  D_801D92A3;
extern s16 D_801D92AA;      /* confirmation highlight Y coordinate */
extern u8  D_801D92B0[];    /* pulsing highlight RGB array (4 * 3 bytes) */
extern u8  D_801D92BF;
extern u8  D_801D92DB;
extern s16 D_801D92E0;
extern s16 D_801D92E2;
extern s16 D_801D92E4;

#define D_80105584 (*(u8 *)(D_800FF748 + 0x5E3C))
#define D_80105592 (*(s16 *)(D_800FF748 + 0x5E4A))
#define D_8010A322 (*(u8 *)(D_800FF748 + 0xABDA))

extern s32 func_800171FC(s32 a0);
extern s32 func_8006EC80(s32 pad_port);
extern s32 func_8006F07C(s32 a0);
extern s32 func_8006F0DC(s32 a0, s32 a1);
extern void func_8006FCFC(s32 a0);
extern void func_800AB620(s32 sfx_id);
extern s32 func_8018E538(s32 a0);
extern s32 func_8018E5B8(s32 a0, s32 a1);
extern u32 func_8018E5EC(u32 cur, s32 min_val, s32 max_val);
extern s32 func_8018EA08(s16 *a0, s32 a1, s32 a2, s16 *a3);
extern void func_8018EC60(s32 a0, u32 a1);
extern void func_80193FB4(s32 a0, s32 a1, s32 a2, s32 a3,
                          s32 s0, s32 s1, s32 s2, s32 s3, s32 s4,
                          s32 s5, s32 s6, s32 s7, s32 s8, s32 s9,
                          s32 s10, s32 s11, s32 s12);
extern void func_801940EC(s32 a0, s32 a1, MenuBoxDesc *box, s32 a3, s32 s0, s32 s1);
extern void func_801941E0(s32 a0, s32 a1, MenuBoxDesc *box, s32 a3, s32 s0, s32 s1);
extern void func_801942E0(s32 a0, u32 a1, s32 a2, s32 a3,
                          s32 s0, s32 s1, s32 s2, s32 s3,
                          s32 s4, s32 s5, s32 s6, s32 s7);

s32 func_801ACE2C(s32 arg0)
{
    u8 state = D_8010A320;
    s32 i;
    s32 pad;

    if (state >= 0x35) {
        return 0;
    }

    switch (state) {
    case 0:
        if (func_8006F07C(0x10) == 0) {
            return 0;
        }
        D_801D86BE = 0;
        D_800AD638 = 0;
        D_8010A320++;
        return 0;

    case 1:
        if (D_801D86BE < 12) {
            u8 step = D_801D86BE;
            D_801D8D78.x = 0;
            D_801D8D78.y = 0;
            D_801D8D78.w = (s16)D_801D273C[step * 2];
            D_801D8D78.h = (s16)D_801D273E[step * 2];
            D_801D8D78.rgba[0] = 0x20;
            D_801D8D78.rgba[1] = 0x20;
            D_801D8D78.rgba[2] = 0x60;
            func_801941E0(2, 0x70000000, &D_801D8D78, 0, 1, 1);
            D_801D86BE++;
            return 0;
        }
        D_8010A320++;
        return 0;

    case 2:
        {
            s32 idx3 = arg0 * 3; /* stride 6 bytes = 3 s16s */
            s16 base_x = D_801D276E[idx3];
            s16 step_x = D_801D2770[idx3];
            s16 label_y = D_801D276C[idx3];
            u8 label_id = D_801D2784[arg0];

            func_80193FB4(3, 0x3021, 0, 0,
                          base_x + (s32)D_800AD638 * step_x, 0x18, 0,
                          0x1000, 0x1000, 0, 0,
                          0x80, 0x80, 0x80, 0, 1, 1);
            func_801942E0(0, D_8011D5F4[label_id], -100, label_y,
                          200, 3, 1, 0, 0, 3, 1, 1);
            D_8010A320++;
            return 0;
        }

    case 3:
        func_8006FCFC(0x36);
        pad = func_8006EC80(0);
        D_8010A5A4 = (s16)pad;
        pad &= 0xFFFF;

        if (pad == 0x1000 || pad == 0x4000) {
            u32 next_cur = func_8018E5EC(D_800AD638, 0, (s32)D_801D2788[arg0] - 1);
            s32 idx3 = arg0 * 3;
            D_800AD638 = (u16)next_cur;
            D_80105592 = (s16)(D_801D276E[idx3] + (s32)(next_cur & 0xFFFF) * D_801D2770[idx3]);
            func_800AB620(0x50D);
            return 0;
        } else if (pad == 0x20) {
            func_800AB620(0x528);
            if ((u32)arg0 < 2) {
                if (D_800AD638 == 0) {
                    D_8010A320 = 0x14;
                } else if (D_800AD638 == 1) {
                    D_8010A320 = 0x1E;
                } else if (D_800AD638 == 2) {
                    D_8010A320 = 0x28;
                }
            } else if (arg0 == 3) {
                if (D_800AD638 == 0) {
                    D_8010A320 = 0x14;
                } else if (D_800AD638 == 1) {
                    D_8010A320 = 0x28;
                }
            } else {
                if (D_800AD638 == 0) {
                    D_8010A320 = 0x1E;
                } else if (D_800AD638 == 1) {
                    D_8010A320 = 0x28;
                }
            }
            return 0;
        } else if (pad == 0x40) {
            func_800AB620(0x529);
            D_8010A320 = 0x32;
            return 0;
        }
        return 0;

    case 20:
        func_8006FCFC(0x36);
        if (func_8006F0DC(0x40, 0) != 0) {
            D_8010A320++;
        }
        return 0;

    case 21:
        func_8018E538(0);
        D_80105584 = 0;
        D_801D8FA0 = 0;
        D_801D92DB = 0;
        func_800171FC(0x81);
        D_8010A322 = 0;
        D_8010A320 = 2;
        return 0;

    case 30:
        D_80105584 = 0;
        D_801D8FA0 = 0;
        D_8010A320++;
        return 0;

    case 31:
        {
            s32 done = 1;
            done &= func_8018EA08(&D_801D92E4, 0, 0x10, &D_801D92E0);
            done &= func_8018EA08(&D_801D92E4 + 1, 0, 0x8, &D_801D92E2);
            if (done != 0) {
                D_801D92DB = 0;
                D_8010A320++;
            }
            return 0;
        }

    case 32:
        if (func_8006F0DC(0x40, 0) == 0) {
            return 0;
        }
        return 2;

    case 40:
        D_801D86BF = 1;
        D_801D8D78.x = 0;
        D_801D8D78.y = 0;
        D_801D8D78.w = 0xC8;
        D_801D8D78.h = 0x50;
        D_801D8D78.rgba[0] = 0;
        D_801D8D78.rgba[1] = 0;
        D_801D8D78.rgba[2] = 0x30;
        func_801941E0(1, 0, &D_801D8D78, 0, 1, 1);

        D_801D8D78.x = -0x32;
        D_801D8D78.y = (s16)(D_801D86BF * 22 - 8);
        D_801D8D78.w = 0x64;
        D_801D8D78.h = 0x10;
        for (i = 0; i < 12; i++) {
            D_801D8D78.rgba[i] = 0;
        }
        func_801940EC(0, 0, &D_801D8D78, 2, 1, 1);

        func_801942E0(0, D_8011D73C, -100, -30, 0xC8, 3, 1, 0, 0, 3, 0, 1);
        func_801942E0(1, D_8011D73C, -100, -8,  0xC8, 3, 1, 6, 0, 12, 0, 1);
        D_8010A320++;
        return 0;

    case 41:
        D_80105584 = 0;
        for (i = 0; i < 4; i++) {
            s32 val = i * 8 + func_8018E5B8(0x40, 1);
            if (val >= 0x40) {
                val = 0x62 - val;
            }
            val >>= 1;
            D_801D92B0[i * 3 + 0] = (u8)(val * 3);
            D_801D92B0[i * 3 + 1] = (u8)val;
            D_801D92B0[i * 3 + 2] = (u8)(val * 4);
        }

        pad = func_8006EC80(0);
        D_8010A5A4 = (s16)pad;
        pad &= 0xFFFF;

        if (pad == 0x1000 || pad == 0x4000) {
            u8 next_yn = (u8)func_8018E5EC(D_801D86BF, 0, 1);
            D_801D86BF = next_yn;
            D_801D92AA = (s16)(next_yn * 22 - 8);
            func_800AB620(0x50D);
            return 0;
        }

        D_801D269D = 1;
        D_801D269C = D_801D86BF;

        if (pad == 0x20) {
            func_800AB620(0x528);
            D_801D269D = 0;
            D_801D8FB8 = 0;
            if (D_801D86BF == 1) {
                D_8010A320++;
            } else {
                D_8010A320 += 2;
            }
        } else if (pad == 0x40) {
            func_800AB620(0x529);
            D_801D269D = 0;
            D_801D8FB8 = 0;
            D_8010A320++;
        }
        return 0;

    case 42:
        D_801D92A3 = 0;
        D_801D92BF = 0;
        D_801D8FA0 = 0;
        func_8018EC60(0, D_8011D5F4[D_801D2784[arg0]]);
        D_8010A320 = 2;
        return 0;

    case 43:
        if (func_8006F0DC(0x10, 0) == 0) {
            return 0;
        }
        return 3;

    case 50:
        D_80105584 = 0;
        D_801D8FA0 = 0;
        D_801D86BE = 0;
        D_8010A320++;
        return 0;

    case 51:
        if (D_801D86BE < 12) {
            u8 step = D_801D86BE;
            D_801D8D78.x = 0;
            D_801D8D78.y = 0;
            D_801D8D78.w = (s16)D_801D273C[step * 2];
            D_801D8D78.h = (s16)D_801D273E[step * 2];
            func_801941E0(2, 0x70000000, &D_801D8D78, 0, 1, 1);
            D_801D86BE++;
            return 0;
        }
        D_801D92DB = 0;
        D_8010A320++;
        return 0;

    case 52:
        return 1;

    default:
        /* 4..19, 22..29, 33..39, 44..49 (.L801AD7C4) */
        return 0;
    }
}
