/*
 * func_800860DC
 *
 * Purpose:
 *   Initializes the halftime / full-time match statistics & scoreboard HUD
 *   screen: spawns UI objects via func_8001CEE0, sets layout bounds in
 *   D_8010A370..D_8010A384 based on SCRATCH+0x32C, populates regular and PK
 *   scores, half-by-half goals, and per-team match statistics (8 stat rows).
 *
 * Arguments:
 *   void
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x2CE (u8)    PK shootout / extra score display active flag (R)
 *     +0x2D1 (u8)    team 0 score (R)
 *     +0x2D2 (u8)    team 1 score (R)
 *     +0x2D3..+0x2D6 (u8[4]) team 0 period/half scores (R)
 *     +0x2D7..+0x2DA (u8[4]) team 1 period/half scores (R)
 *     +0x2DB (u8)    team 0 PK score (R)
 *     +0x2DC (u8)    team 1 PK score (R)
 *     +0x2DF (u8)    number of completed periods/halves (R)
 *     +0x32C (s32)   display width/layout selector (R)
 *   Globals:
 *     D_800CA5E0 (u16[]) table indexed by D_800E81FC (R)
 *     D_800E81FC (u8)    language/variant index (R)
 *     D_80108610..D_8010861E (u8[2] each) per-side match stats arrays (R)
 *     D_8010A370..D_8010A384 (s16) HUD panel coordinates/dimensions (W)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   UI layout function (screen pixel coordinates, no world-space units).
 *   Scores >= 100 are clamped to 99; 0x5A/0x5B/0x61/0x66 are sprite/glyph IDs.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned short D_800CA5E0[];
extern unsigned char D_800E81FC;
extern unsigned char D_800FF748[];
extern signed short D_8010A370;
extern signed short D_8010A372;
extern signed short D_8010A374;
extern signed short D_8010A376;
extern signed short D_8010A378;
extern signed short D_8010A37A;
extern signed short D_8010A37C;
extern signed short D_8010A384;

extern void func_8001CEE0(int slot, int id, int a2, int a3, int s0, int s1, int s2, int s3);
extern unsigned char *func_8001D004(int id);
extern void func_8005C49C(void);
extern void func_80086930(int base_x, unsigned char *entry, unsigned char val, int side, int glyph_base, int flag);

void func_800860DC(void)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *obj;
    /* UNCLEAR: width_val is uninitialized in asm if SCRATCH+0x32C misses all switch cases */
    signed short width_val = 0;
    int score0, score1;
    int i;
    int off_c0, off_108, off_138, off_168, off_198, off_1c8, off_1f8, off_228;

    func_8001CEE0(1, D_800CA5E0[D_800E81FC], 0, 0, 0, 6, 0, 1);
    func_8001CEE0(3, 0x8B, 0, 0, 0, 0xA, 0, 1);
    func_8001CEE0(4, 0x8C, 0, 0, 0, 0xA, 0, 1);
    func_8001CEE0(5, 0x8D, 0, 0, 0, 0xA, 0, 1);
    func_8001CEE0(6, 0x91, 0, 0, 0, 0xA, 0, 1);
    func_8001CEE0(7, 0x92, 0x60000000, 0, 0, 0xA, 0, 1);

    func_8005C49C();

    D_8010A370 = -0x54;
    D_8010A372 = 0x54;
    D_8010A376 = -0x1C;
    D_8010A374 = -0x1C;

    switch (SW(scratch, 0x32C)) {
    case 0xDB:
        width_val = 0x40;
        break;
    case 0x12A:
        width_val = 0x80;
        break;
    case 0x15F:
    case 0x163:
        width_val = 0x90;
        break;
    case 0x180:
        width_val = 0xB0;
        break;
    case 0x198:
        width_val = 0xC8;
        break;
    case 0x1D0:
        width_val = 0xF0;
        break;
    case 0x200:
        width_val = 0x118;
        break;
    }

    D_8010A37A = width_val;
    D_8010A378 = width_val;
    D_8010A384 = -0x20;
    D_8010A37C = -0x20;

    if (B(scratch, 0x2CE) != 0) {
        int pk0, pk1;

        func_8001CEE0(2, 0x90, 0, 0, 0, 0xA, 0, 1);
        obj = func_8001D004(0x90);
        pk0 = B(scratch, 0x2DB);
        pk1 = B(scratch, 0x2DC);

        if (pk0 >= 10) {
            B(obj, 0xA) = 0x5A;
            S(obj, 0x12) = -0x26;
            B(obj, 0x3) = (unsigned char)((pk0 / 10) << 3);
        } else {
            B(obj, 0xA) = 0x5B;
            S(obj, 0x12) = -0x2A;
        }
        B(obj, 0xF) = (unsigned char)((pk0 % 10) << 3);

        if (pk1 >= 10) {
            B(obj, 0x22) = 0x5A;
            S(obj, 0x2A) = 0x26;
            B(obj, 0x1B) = (unsigned char)((pk1 / 10) << 3);
        } else {
            B(obj, 0x22) = 0x5B;
            S(obj, 0x2A) = 0x22;
        }
        B(obj, 0x27) = (unsigned char)((pk1 % 10) << 3);
    }

    obj = func_8001D004(0x8B);
    score0 = B(scratch, 0x2D1);
    if ((unsigned int)score0 >= 100) {
        score0 = 99;
    }
    score1 = B(scratch, 0x2D2);
    if ((unsigned int)score1 >= 100) {
        score1 = 99;
    }

    if (score0 >= 10) {
        B(obj, 0xA) = 0x66;
        B(obj, 0x3) = (unsigned char)((score0 / 10) << 4);
        S(obj, 0x12) = -0x26;
        B(obj, 0xF) = (unsigned char)((score0 % 10) << 4);
    } else {
        B(obj, 0xA) = 0x5B;
        S(obj, 0x12) = -0x2D;
        B(obj, 0xF) = (unsigned char)(score0 << 4);
    }

    obj = func_8001D004(0x8C);
    if (score1 >= 10) {
        B(obj, 0xA) = 0x66;
        B(obj, 0x3) = (unsigned char)((score1 / 10) << 4);
        S(obj, 0x12) = 0x26;
        B(obj, 0xF) = (unsigned char)((score1 % 10) << 4);
    } else {
        B(obj, 0xA) = 0x5B;
        S(obj, 0x12) = 0x1F;
        B(obj, 0xF) = (unsigned char)(score1 << 4);
    }

    for (i = 0; i < 2; i++) {
        signed short y_pos;
        obj = func_8001D004(0x8B + i);
        y_pos = (B(scratch, 0x2CE) != 0) ? -0x2C : -0x38;
        S(obj, 0x8) = y_pos;
        S(obj, 0x14) = y_pos;
    }

    obj = func_8001D004(0x91);
    for (i = 0; i < 4; i++) {
        unsigned char *entry = obj + i * 0x30;
        if ((int)B(scratch, 0x2DF) < i) {
            B(entry, 0xA) = 0x5B;
            B(entry, 0x16) = 0x5B;
        } else {
            B(entry, 0xA) = 0x61;
            B(entry, 0x16) = 0x61;
        }
        func_80086930(0xA, entry, B(scratch, 0x2D3 + i), 0, 0x61, 0);
    }

    for (i = 0; i < 4; i++) {
        unsigned char *entry = obj + i * 0x30;
        if ((int)B(scratch, 0x2DF) < i) {
            B(entry, 0x22) = 0x5B;
            B(entry, 0x2E) = 0x5B;
        } else {
            B(entry, 0x22) = 0x61;
            B(entry, 0x2E) = 0x61;
        }
        func_80086930(0xA, entry + 0x18, B(scratch, 0x2D7 + i), 1, 0x61, 0);
    }

    off_c0 = 0xC0;
    off_108 = 0x108;
    off_138 = 0x138;
    off_168 = 0x168;
    off_198 = 0x198;
    off_1c8 = 0x1C8;
    off_1f8 = 0x1F8;
    off_228 = 0x228;

    for (i = 0; i < 2; i++) {
        unsigned char side = (unsigned char)i;
        unsigned char *stats_base = D_800FF748 + i;

        /* D_80108610..D_8010861E via D_800FF748 + 0x10000 - 0x7138..0x712A */
        func_80086930(0x64, obj + off_c0, B(stats_base, 0x8EC8), side, 0x5A, 1);
        off_c0 += 0x24;

        func_80086930(0xA, obj + off_108, B(stats_base, 0x8ECA), side, 0x5A, 1);
        off_108 += 0x18;

        func_80086930(0xA, obj + off_138, B(stats_base, 0x8ECC), side, 0x5A, 1);
        off_138 += 0x18;

        func_80086930(0xA, obj + off_168, B(stats_base, 0x8ED0), side, 0x16, 0);
        off_168 += 0x18;

        func_80086930(0xA, obj + off_198, B(stats_base, 0x8ECE), side, 0x17, 0);
        off_198 += 0x18;

        func_80086930(0xA, obj + off_1c8, B(stats_base, 0x8ED4), side, 0x5A, 1);
        off_1c8 += 0x18;

        func_80086930(0xA, obj + off_1f8, B(stats_base, 0x8ED2), side, 0x5A, 1);
        off_1f8 += 0x18;

        func_80086930(0xA, obj + off_228, B(stats_base, 0x8ED6), side, 0x5A, 1);
        off_228 += 0x18;
    }
}
