/*
 * func_801A30F4 (overlay: select)
 *
 * Purpose:
 *   Renders the two-team ability/stat comparison bars (5 bars per side) on the
 *   team select screen and advances the bar reveal animation counters
 *   (D_801DA060 for team 0 / left, D_801DA064 for team 1 / right, clamped to 8).
 *   Also marks active bar slots in D_801D9217 / D_801D92A3 and copies gradient
 *   RGB colors from D_801D1F84..D_801D1F89 into the quad descriptor D_801D8D90.
 *
 * Arguments:
 *   arg0 (u8 in $a0): draw/color parameter passed to func_801940EC for left bars
 *                     (forced to 0 if D_801D8B17 == 0 and the selected team's
 *                     byte at D_8010866D[team * 36] == 0xFF)
 *   arg1 (u8 in $a1): draw/color parameter passed to func_801940EC for right bars
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
    s16 x;          /* 0x00 */
    s16 y;          /* 0x02 */
    s16 w;          /* 0x04 */
    s16 h;          /* 0x06 */
    u8  rgb[4][3];  /* 0x08..0x13: 4 corner RGB colors */
} SelectBarQuad;

extern u8  D_800FF748[];    /* base symbol for select/work RAM offsets */
extern u8  D_801D1F84;      /* bar gradient color 1 R */
extern u8  D_801D1F85;      /* bar gradient color 1 G */
extern u8  D_801D1F86;      /* bar gradient color 1 B */
extern u8  D_801D1F87;      /* bar gradient color 0 R */
extern u8  D_801D1F88;      /* bar gradient color 0 G */
extern u8  D_801D1F89;      /* bar gradient color 0 B */
extern u8  D_801D8042[];    /* stat values table (indices 6..10 = left, 11..15 = right) */
extern u8  D_801D8A70;      /* index into 0x8010A2C8 table */
extern u8  D_801D8B17;      /* flag disabling team-lock check when non-zero */
extern SelectBarQuad D_801D8D90; /* quad descriptor (0x801D8D90..0x801D8DA3) */
extern u8  D_801D9217[];    /* left-side bar slot state array (offset 0x134, stride 0x1C) */
extern u8  D_801D92A3[];    /* right-side bar slot state array (offset 0x134, stride 0x1C) */
extern s32 D_801DA060;      /* left bar reveal animation counter (0..8) */
extern s32 D_801DA064;      /* right bar reveal animation counter (0..8) */

/* Base-relative pointers into work RAM (D_800FF748 + 0xAB80 / + 0x8F25) */
#define D_8010A2C8 ((u8 *)(D_800FF748 + 0xAB80))
#define D_8010866D ((u8 *)(D_800FF748 + 0x8F25))

extern void func_801940EC(s32 slot, s32 a1, SelectBarQuad *quad, s32 a3, s32 a4, u32 a5);

void func_801A30F4(s32 arg0, s32 arg1)
{
    s32 i;
    s32 stride_off;
    s32 stat_idx;
    s16 y_pos;
    s32 width;
    u8 r0, g0, b0;
    u8 r1, g1, b1;

    /* Mark the 5 bar slots (stride 0x1C starting at +0x134) active if counter != 0 */
    stride_off = 0x134;
    for (i = 0; i < 5; i++) {
        if (D_801DA060 != 0) {
            D_801D9217[stride_off] = 1;
        }
        if (D_801DA064 != 0) {
            D_801D92A3[stride_off] = 1;
        }
        stride_off += 0x1C;
    }

    /* Set left-side bar gradient colors (color0 at left/bottom-left, color1 at right) */
    r0 = D_801D1F87;
    g0 = D_801D1F88;
    b0 = D_801D1F89;
    r1 = D_801D1F84;
    g1 = D_801D1F85;
    b1 = D_801D1F86;

    D_801D8D90.rgb[0][0] = r0;
    D_801D8D90.rgb[0][1] = g0;
    D_801D8D90.rgb[0][2] = b0;
    D_801D8D90.rgb[1][0] = r1;
    D_801D8D90.rgb[1][1] = g1;
    D_801D8D90.rgb[1][2] = b1;
    D_801D8D90.rgb[2][0] = r0;
    D_801D8D90.rgb[2][1] = g0;
    D_801D8D90.rgb[2][2] = b0;
    D_801D8D90.rgb[3][0] = r1;
    D_801D8D90.rgb[3][1] = g1;
    D_801D8D90.rgb[3][2] = b1;

    /* UNCLEAR: Exact meaning of D_8010A2C8[D_801D8A70] and D_8010866D[idx * 36];
     * appears to clear arg0 when D_801D8B17 == 0 and the entry byte is 0xFF.
     */
    if (D_801D8B17 == 0) {
        u8 team_idx = D_8010A2C8[D_801D8A70];
        if (D_8010866D[team_idx * 36] == 0xFF) {
            arg0 = 0;
        }
    }

    /* Draw 5 left-side stat bars (D_801D8042[6..10]) */
    stat_idx = 6;
    y_pos = -0x2A;
    for (i = 0; i < 5; i++) {
        u8 stat_val = D_801D8042[stat_idx];
        s32 slot = stat_idx;

        stat_idx++;
        D_801D8D90.x = -0x21;
        D_801D8D90.y = y_pos;
        D_801D8D90.h = 3;
        width = (stat_val * 2 + 12) - ((8 - D_801DA060) * 2);
        D_801D8D90.w = (s16)width;

        func_801940EC(slot, 0, &D_801D8D90, 2, 0, (u32)(arg0 & 0xFF));
        y_pos += 6;
    }

    /* Set right-side bar gradient colors (mirrored: color1 then color0) */
    r1 = D_801D1F84;
    g1 = D_801D1F85;
    b1 = D_801D1F86;
    r0 = D_801D1F87;
    g0 = D_801D1F88;
    b0 = D_801D1F89;

    D_801D8D90.rgb[0][0] = r1;
    D_801D8D90.rgb[0][1] = g1;
    D_801D8D90.rgb[0][2] = b1;
    D_801D8D90.rgb[1][0] = r0;
    D_801D8D90.rgb[1][1] = g0;
    D_801D8D90.rgb[1][2] = b0;
    D_801D8D90.rgb[2][0] = r1;
    D_801D8D90.rgb[2][1] = g1;
    D_801D8D90.rgb[2][2] = b1;
    D_801D8D90.rgb[3][0] = r0;
    D_801D8D90.rgb[3][1] = g0;
    D_801D8D90.rgb[3][2] = b0;

    /* Draw 5 right-side stat bars (D_801D8042[11..15]), right-aligned at x = 0x21 */
    stat_idx = 11;
    y_pos = -0x2A;
    for (i = 0; i < 5; i++) {
        u8 stat_val = D_801D8042[stat_idx];
        s32 slot = stat_idx;

        stat_idx++;
        D_801D8D90.y = y_pos;
        D_801D8D90.h = 3;
        width = (stat_val * 2 + 12) - ((8 - D_801DA064) * 2);
        D_801D8D90.x = (s16)(0x21 - width);
        D_801D8D90.w = (s16)width;

        func_801940EC(slot, 0, &D_801D8D90, 2, 0, (u32)(arg1 & 0xFF));
        y_pos += 6;
    }

    /* Advance and clamp both animation counters to 8 */
    D_801DA060++;
    if (D_801DA060 >= 9) {
        D_801DA060 = 8;
    }

    D_801DA064++;
    if (D_801DA064 >= 9) {
        D_801DA064 = 8;
    }
}
