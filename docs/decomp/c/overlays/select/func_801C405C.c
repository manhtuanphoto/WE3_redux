/*
 * func_801C405C (overlay: select)
 *
 * Purpose:
 *   Animates the opening (`arg0 != 0`, step 0 -> 0x1E) or closing (`arg0 == 0`,
 *   step 0x1E -> 0) transition of a tournament/cup/stadium info panel in the
 *   select overlay, driven by signed step counter D_801D429D (`D_801D429C + 1`).
 *   Updates sprite X-coordinates in 0x7000 and 0x7003, fades/tints banner
 *   slots 0x37..0x39 and panel slots 0xA..0x1C, expands/collapses the backdrop
 *   box D_801D8D78 (up to 320x34), and displays the title string D_801D5848[D_801D4298]
 *   once the open animation completes.
 *
 * Arguments:
 *   arg0 (u8 in $a0): non-zero = open/slide-in animation; 0 = close/slide-out animation
 *
 * Return value:
 *   1 when the animation sequence finishes (and D_801D429D resets to -1);
 *   0 while still animating.
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef signed short   s16;
typedef unsigned int   u32;
typedef signed int     s32;

/* PS1 Scratchpad RAM (0x1F800000..0x1F8003FF) */
#define SPAD_2A2 (*(volatile u8 *)0x1F8002A2)
#define SPAD_330 (*(volatile u8 *)0x1F800330)

typedef struct {
    s16 x;          /* 0x00 */
    s16 y;          /* 0x02 */
    s16 w;          /* 0x04 */
    s16 h;          /* 0x06 */
    u8  rgba[12];   /* 0x08..0x13: 4 corner RGB colors */
} MenuBoxDesc;

extern u8  D_8010A5B1;      /* mode/flags byte (bits 2..3 select palette/mode) */
extern s8  D_801D4298;      /* selected panel/item index */
extern u8  D_801D429C;      /* base symbol; D_801D429C + 1 is the animation step */
extern u32 D_801D5848[];    /* string pointer table indexed by D_801D4298 */
extern MenuBoxDesc D_801D8D78;

/* D_801D429D: animation step (-1 = idle, 0..0x1E = active frame) */
#define D_801D429D (*(volatile s8 *)(&D_801D429C + 1))

extern u8  *func_8001D004(s32 id);
extern void func_8018E538(s32 a0);
extern void func_80193FB4(s32 slot, s32 id, u32 a2, s32 a3,
                          s32 s0, s32 s1, s32 s2, s32 s3, s32 s4,
                          s32 s5, s32 s6, s32 s7, s32 s8, s32 s9,
                          s32 s10, s32 s11, s32 s12);
extern void func_801941E0(s32 slot, s32 a1, MenuBoxDesc *box, s32 a3, s32 s0, s32 s1);
extern void func_801942E0(s32 slot, u32 text, s32 x, s32 y,
                          s32 max_w, s32 font_mode, s32 s2, s32 s3,
                          s32 s4, s32 align_mode, s32 s6, s32 s7);
extern void func_801C3888(s32 a0);

u32 func_801C405C(u8 arg0)
{
    u8  done = 0;
    u8  mode_bits;
    s32 step;
    u8  slide_x;
    u8 *spr_7000;
    u8 *spr_7003;
    s32 i;
    s32 bright_banner;
    u32 clut_param;
    u8  bright_panel;
    u32 panel_flags0;
    u32 panel_flags1;

    SPAD_2A2 = 0;
    mode_bits = (D_8010A5B1 & 0x0C) >> 2;

    if (D_801D429D == -1) {
        func_8018E538(0);
        SPAD_330 = 0;

        func_80193FB4(0x36, 0x3003, 0, 0,
                      0x40, 0x18, 0,
                      0x1000, 0x1000, 0, 0,
                      0x80, 0x80, 0x80, 0, 5, 0);

        D_801D429D = (arg0 != 0) ? 0 : 0x1E;

        D_801D8D78.x = 0;
        D_801D8D78.y = 0x4E;
        D_801D8D78.w = 2;
        D_801D8D78.h = 2;
        for (i = 0; i < 4; i++) {
            D_801D8D78.rgba[i * 3 + 0] = 0x20;
            D_801D8D78.rgba[i * 3 + 1] = 0x20;
            D_801D8D78.rgba[i * 3 + 2] = 0x60;
        }
    }

    step = D_801D429D;
    slide_x = (step < 0x10) ? (u8)(0x40 - step * 4) : 0;

    /* Update sprite X-coordinates in table 0x7000 (12-byte stride, s16 at +0x08) */
    spr_7000 = func_8001D004(0x7000);
    for (i = 0; i < 3; i++) {
        *(s16 *)(spr_7000 + i * 12 + 8) = (s16)(0x68 + slide_x);
    }
    for (i = 3; i < 6; i++) {
        *(s16 *)(spr_7000 + i * 12 + 8) = (s16)(-0x80 - slide_x);
    }
    for (i = 6; i < 9; i++) {
        *(s16 *)(spr_7000 + i * 12 + 8) = (s16)(-0x58 - slide_x);
    }
    *(s16 *)(spr_7000 + 0x8C) = (s16)(-0x5D - slide_x);
    *(s16 *)(spr_7000 + 0x74) = (s16)(-0x5D - slide_x);
    *(s16 *)(spr_7000 + 0xA4) = (s16)(-0x55 - slide_x);
    *(s16 *)(spr_7000 + 0x98) = (s16)(-0x55 - slide_x);
    *(s16 *)(spr_7000 + 0x80) = (s16)(-0x55 - slide_x);
    for (i = 0xE; i < 0x11; i++) {
        *(s16 *)(spr_7000 + i * 12 + 8) = (s16)(0x65 + slide_x);
    }

    func_80193FB4(2, 0x7000, 0, 0,
                  0, 9, 0,
                  0x1000, 0x1000, 0, 0,
                  0x80, 0x80, 0x80, 0, 4, 1);

    /* Update sprite X-coordinates in table 0x7003 */
    spr_7003 = func_8001D004(0x7003);
    *(s16 *)(spr_7003 + 0x2C) = (s16)(-0x64 - slide_x);
    *(s16 *)(spr_7003 + 0x08) = (s16)(-0x64 - slide_x);
    *(s16 *)(spr_7003 + 0x20) = (s16)(-0x6C - slide_x);
    *(s16 *)(spr_7003 + 0x14) = (s16)(-0x6C - slide_x);

    func_80193FB4(1, 0x7003, 0, 0,
                  0, 0x1A, 0,
                  0x1000, 0x1000, 0, 0,
                  0x80, 0x80, 0x80, 0, 4, 1);

    step = D_801D429D;
    bright_banner = (step < 0x11) ? ((-0x80 - step * 4) & 0xFF) : 0x40;
    clut_param = (u32)mode_bits | 0xFC;

    func_80193FB4(0x37, 0x3000, 0x01000000U, 0,
                  0, 0x0D, (s32)clut_param, 0x1000, 0x1000, 0, 0,
                  bright_banner, bright_banner, bright_banner, 0, 6, 1);
    func_80193FB4(0x38, 0x3001, 0x01000000U, 0,
                  0, 0x0E, (s32)clut_param, 0x1000, 0x1000, 0, 0,
                  bright_banner, bright_banner, bright_banner, 0, 6, 1);
    func_80193FB4(0x39, 0x3002, 0x01000000U, 0,
                  0, 0x0F, (s32)clut_param, 0x1000, 0x1000, 0, 0,
                  bright_banner, bright_banner, bright_banner, 0, 6, 1);

    step = D_801D429D;
    if (step < 0x10) {
        bright_panel = (u8)(step * 16);
        if (mode_bits == 1) {
            panel_flags0 = 0x70000000U;
            panel_flags1 = 0x70000000U;
        } else {
            panel_flags0 = 0x50000000U;
            panel_flags1 = 0x50000000U;
        }
        func_801C3888(0x0A);
    } else if (step < 0x17) {
        SPAD_2A2 = 0x80;
        if (mode_bits == 1) {
            s32 cur = D_801D429D;
            if (cur < 0x14) {
                bright_panel = (u8)((cur - 0x13) * 16 - 0x50);
            } else {
                bright_panel = (u8)(0xB0 - (cur - 0x13) * 16);
            }
        } else {
            bright_panel = (u8)(-0x10 - (D_801D429D - 0x10) * 16);
        }
        panel_flags1 = 0x40000000U;
        panel_flags0 = 0;
        func_801C3888(D_801D4298);
    } else {
        SPAD_2A2 = 0x80;
        bright_panel = 0x80;
        panel_flags1 = 0x40000000U;
        panel_flags0 = 0;
    }

    func_80193FB4(0x0A, 0x7002, panel_flags0, 0,
                  0, 0x0C, 0, 0x1000, 0x1000, 0, 0,
                  bright_panel, bright_panel, bright_panel, 0, 4, 1);
    func_80193FB4(0x14, 0x7001, panel_flags1, 0,
                  0, 0x0C, 0, 0x1000, 0x1000, 0, 0,
                  bright_panel, bright_panel, bright_panel, 0, 4, 1);

    for (i = 0; i < 8; i++) {
        func_80193FB4(i + 0x0B, i + 0x700C, panel_flags0, 0,
                      0, 0x0C, 0, 0x1000, 0x1000, 0, 0,
                      bright_panel, bright_panel, bright_panel, 0, 4, 1);
        func_80193FB4(i + 0x15, i + 0x7004, panel_flags1, 0,
                      0, 0x0C, 0, 0x1000, 0x1000, 0, 0,
                      bright_panel, bright_panel, bright_panel, 0, 4, 1);
    }

    step = D_801D429D;
    if (step >= 0x0E) {
        D_801D8D78.w = (s16)((step - 0x0E) * 0x27 + 8);
        step = D_801D429D;
        if (step >= 0x16) {
            D_801D8D78.h = (s16)((step - 0x16) * 4 + 2);
        }
    }

    if ((u16)D_801D8D78.w >= 0x140) {
        D_801D8D78.w = 0x140;
    }
    if ((u16)D_801D8D78.h >= 0x22) {
        D_801D8D78.h = 0x22;
    }

    func_801941E0(0, 0, &D_801D8D78, 0, 2, (D_801D429D >= 0x0E) ? 1 : 0);

    if (arg0 != 0) {
        D_801D429D++;
    } else {
        D_801D429D--;
    }

    if ((u8)D_801D429D >= 0x1F) {
        if (D_801D429D < 0) {
            func_8018E538(0);
            func_80193FB4(0x37, 0x3000, 0x01000000U, 0,
                          0, 0x0D, (s32)clut_param, 0x1000, 0x1000, 0, 0,
                          0x80, 0x80, 0x80, 0, 6, 1);
            func_80193FB4(0x38, 0x3001, 0x01000000U, 0,
                          0, 0x0E, (s32)clut_param, 0x1000, 0x1000, 0, 0,
                          0x80, 0x80, 0x80, 0, 6, 1);
            func_80193FB4(0x39, 0x3002, 0x01000000U, 0,
                          0, 0x0F, (s32)clut_param, 0x1000, 0x1000, 0, 0,
                          0x80, 0x80, 0x80, 0, 6, 1);
        } else {
            SPAD_2A2 = 0x80;
            func_801942E0(0, D_801D5848[(u8)D_801D4298], -0x9C, 0x3E,
                          0x138, 0, 0, 0, 0, 3, 1, 1);
        }
        D_801D429D = -1;
        done = 1;
    }

    return done;
}
