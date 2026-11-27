/*
 * func_801BDE5C (overlay: select)
 *
 * Purpose:
 *   Draws header/status info and team/match bracket or standings sprites for
 *   the select overlay depending on mode (`arg0 & 0xFF`).
 *   - Always formats two 2-character ASCII fields into D_801D4274..D_801D427A
 *     from D_8010866A + 1 (tens/ones) and bit 6 of D_80108667 ("15" vs "30"),
 *     and draws sprite slot 0 (ID 0x3020).
 *   - Mode 0 or 1: resets D_801D4281 = 0, calls func_801A8E08(), draws slot
 *     0x39 (ID 0x5009), draws text at D_801D4274, and draws a translucent box
 *     via func_801940EC.
 *   - Mode 2 or 3: clears 16 x 45-byte text buffers at D_801D94E8, configures
 *     16 team emblem/flag sprite entries at 0x3109 / 0x310B / 0x310A,
 *     optionally formats 16 score/rank strings (mode 2 only) from D_8010A2A8,
 *     and registers 11 overlay UI sprite slots (0x10..0x18, 0x33, 0x34).
 *
 * Arguments:
 *   arg0 (s32 in $a0): display mode byte (0..3 active; >= 4 no-op)
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
    u8  rgba[12];   /* 0x08..0x13 */
} SelectBoxDesc;

extern u8  D_800FF748[];    /* base symbol for select/work RAM offsets */
extern u8  D_801D4274;      /* formatted header string buffer (tens digit) */
extern u8  D_801D4275;      /* ones digit */
extern u8  D_801D4279;      /* '1' or '3' */
extern u8  D_801D427A;      /* '5' or '0' */
extern u8  D_801D4281;      /* string terminator byte (+0x0D from D_801D4274) */
extern SelectBoxDesc D_801D8D78;
extern u8  D_801D8F9C[];    /* SelectTextSlot +0x0C field (24-byte stride) */
extern u8  D_801D94E8[];    /* 16 x 45-byte string buffer array */

/* Base-relative pointers into work RAM (D_800FF748 + offset) */
#define D_80108667 (*(u8 *)(D_800FF748 + 0x8F1F)) /* flags byte (bit 6 selects 15/30) */
#define D_8010866A (*(u8 *)(D_800FF748 + 0x8F22)) /* 0-based counter/round index */
#define D_8010866D ((u8 *)(D_800FF748 + 0x8F25))  /* team info table (stride 36 bytes) */
#define D_8010A298 ((u8 *)(D_800FF748 + 0xAB50))  /* team slot remap table (mode 2) */
#define D_8010A2A8 ((u8 *)(D_800FF748 + 0xAB60))  /* 16-entry numeric values (mode 2) */
#define D_8010A2C8 ((u8 *)(D_800FF748 + 0xAB80))  /* 16-entry team ID list */

extern u8  *func_8001D004(s32 id);
extern void func_80193FB4(s32 slot, s32 id, s32 a2, s32 a3,
                          s32 s0, s32 s1, s32 s2, s32 s3, s32 s4,
                          s32 s5, s32 s6, s32 s7, s32 s8, s32 s9,
                          s32 s10, s32 s11, s32 s12);
extern void func_801940EC(s32 slot, u32 flags, SelectBoxDesc *box, s32 a3, s32 s0, s32 s1);
extern void func_801942E0(s32 slot, u8 *text, s32 x, s32 y,
                          s32 max_w, s32 font_mode, s32 s2, s32 s3,
                          s32 s4, s32 align_mode, s32 s6, s32 s7);
extern void func_801A8E08(void);
extern void func_801A93CC(u8 *dst, u32 val, u32 pad_flag);
extern void func_801BFCDC(u8 *entry, u32 team_flag_id, s32 a2);

void func_801BDE5C(s32 arg0)
{
    s32 val;
    s32 tens;
    s32 ones;
    u8  mode;
    s32 i;
    s32 j;

    val = (s32)D_8010866A + 1;
    tens = val / 10;
    D_801D4274 = (tens == 0) ? (u8)' ' : (u8)(tens + '0');

    ones = val - tens * 10;
    D_801D4275 = (u8)(ones + '0');

    if ((D_80108667 >> 6) & 1) {
        D_801D4279 = '3';
        D_801D427A = '0';
    } else {
        D_801D4279 = '1';
        D_801D427A = '5';
    }

    func_80193FB4(0, 0x3020, 0, 0,
                  0, 0x1B, 0,
                  0x1000, 0x1000, 0, 0,
                  0x80, 0x80, 0x80, 0, 3, 0);

    mode = (u8)(arg0 & 0xFF);
    if (mode == 1) {
        /* Note: asm writes 0x20 to D_801D4281 then immediately clears it to 0 in delay slot */
        D_801D4281 = ' ';
    }

    if (mode <= 1) {
        D_801D4281 = 0;
        func_801A8E08();

        func_80193FB4(0x39, 0x5009, 0, 0,
                      0, 9, 0xBA,
                      0x1000, 0x1000, 0, 0,
                      0x80, 0x80, 0x80, 0, 6, 1);

        func_801942E0(0, &D_801D4274, -0xA7, -0x49,
                      0x14E, 3, 1, 0, 0, 2, 0, 1);

        D_801D8D78.x = -0x54;
        D_801D8D78.y = -0x48;
        D_801D8D78.w = 0xA8;
        D_801D8D78.h = 0x10;
        D_801D8D78.rgba[0] = 0x80;
        D_801D8D78.rgba[1] = 0x80;
        D_801D8D78.rgba[2] = 0x40;

        func_801940EC(0, 0x60000000U, &D_801D8D78, 0, 1, 1);
        return;
    }

    if (mode >= 4) {
        return;
    }

    /* Mode 2 or 3: clear 16 x 45-byte text buffers */
    for (i = 0; i < 16; i++) {
        u8 *buf = &D_801D94E8[i * 45];
        for (j = 0; j < 45; j++) {
            buf[j] = 0;
        }
    }

    /* Configure 16 sprite entries in 0x3109 and 0x310B */
    for (i = 0; i < 16; i++) {
        u8  slot_idx = (mode == 2) ? D_8010A298[i] : (u8)i;
        u8  team_id  = D_8010A2C8[slot_idx];
        u32 flag_id  = D_8010866D[(s32)team_id * 36];
        u8 *base_09  = func_8001D004(0x3109);
        u8 *base_0b;
        u8 *entry_0b;
        u32 quot;
        u32 rem;

        func_801BFCDC(base_09 + i * 12, flag_id, 0);

        base_0b  = func_8001D004(0x310B);
        entry_0b = base_0b + i * 12;
        quot     = flag_id / 18;
        rem      = flag_id - quot * 18;

        entry_0b[2]   = 0x0E;
        entry_0b[0xA] = 0x95;
        entry_0b[3]   = (u8)((quot & 0xFF) << 6);
        entry_0b[4]   = (u8)(rem * 14);
    }

    /* Configure 16 sprite entries in 0x310A */
    for (i = 0; i < 16; i++) {
        u8  team_id = D_8010A2C8[i];
        u8  flag_id = D_8010866D[(s32)team_id * 36];
        u8 *base_0a = func_8001D004(0x310A);

        func_801BFCDC(base_0a + i * 12, flag_id, 0);
    }

    /* Mode 2: format and display 16 numeric labels (text slots 4..19) */
    if (mode == 2) {
        for (i = 0; i < 16; i++) {
            u8 *buf     = &D_801D94E8[i * 45];
            u8  num_val = D_8010A2A8[i];
            u8  pad_val = (num_val < 10) ? 5 : 0;
            s32 text_slot = i + 4;

            buf[0] = 9;
            buf[1] = pad_val;
            func_801A93CC(&buf[2], num_val, pad_val);

            func_801942E0(text_slot, buf, -0xAD, i * 22 - 0x9C,
                          0x100, 1, 0, 0, 0, 6, 1, 1);

            /* Sets SelectTextSlot[text_slot].pad_0c[0] = 0x95 */
            D_801D8F9C[text_slot * 24] = 0x95;
        }
    }

    func_80193FB4(0x10, 0x310B, 0, 0, 2, 0x19, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 2, 1);
    func_80193FB4(0x11, 0x3103, 0, 0, 0, 0x1D, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 3, 1);
    func_80193FB4(0x12, 0x3109, 0, 0, 0, 0x1D, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 1, 1);
    func_80193FB4(0x13, 0x310A, 0, 0, 0, 0x1D, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 3, 0);
    func_80193FB4(0x14, 0x3104, 0, 0, 0, 0x1D, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 5, 0);
    func_80193FB4(0x15, 0x3105, 0, 0, 0, 0x1D, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 5, 0);
    func_80193FB4(0x16, 0x3108, 0, 0, 0, 0x1D, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 1, 0);
    func_80193FB4(0x17, 0x3106, 0, 0, -1, 0x1D, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 5, 0);
    func_80193FB4(0x18, 0x3107, 0, 0, 0, 0x1D, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 5, 0);
    func_80193FB4(0x33, 0x3101, 0, 0, 0, 0x09, 0xBA, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 6, 1);
    func_80193FB4(0x34, 0x3100, 0, 0, 0, 0x09, 0xBA, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 6, 1);
}
