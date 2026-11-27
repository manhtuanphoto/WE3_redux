/*
 * func_800204D8
 *
 * Purpose:
 *   Initializes match HUD UI widgets/sprites (scoreboard, clock, team names,
 *   flags, player name/stamina bars, radar/overlay elements) by registering
 *   entries via func_8001CEE0 and configuring the resulting widget structs
 *   returned by func_8001D004. Configures visibility/color/position based on
 *   game state (Scratchpad+0x298), mode (+0x2E1), side/time flags (+0x2E0,
 *   +0x2E2), and D_8010A5B2 / D_8010A5A8 option bitmasks, then calls HUD
 *   layout/init helpers.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Actor / global table fields read:
 *   D_8010A5B2 (u8) HUD display option flags (bits 0x03, 0x04, 0x08, 0x10),
 *   D_8010A5A8 + 0x0A (u8) HUD option flags (bits 0x04, 0x20),
 *   D_800C6B48[] / D_800C6B49[] (u8) per-match-type sprite/label index tables
 *
 * Global fields written:
 *   D_8010A5A2 (u16) cleared to 0,
 *   D_800FF748 + 0x64CC + i * 0x28 (u8) cleared for i = 0..6 when +0x298 == 3,
 *   UI widget fields (+0x01, +0x03, +0x04, +0x0A, +0x16, +0x26, +0x28, +0x2E,
 *   +0x32, +0x34, +0x3A) on objects returned by func_8001D004
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x298 (u8) game state (3 checked),
 *   +0x2DD / +0x2DE (u8) team 0 / team 1 scores,
 *   +0x2DF (u8) match time/length setting (multiplied by 40),
 *   +0x2E0 (u8) side swap/home-away index (0 or 1),
 *   +0x2E1 (u8) mode (5 = PK / shootout / special mode),
 *   +0x2E2 (u8) match half/period index (0..4)
 *
 * Scratchpad fields written:
 *   +0x244 / +0x245 (u8) HUD/scoreboard team slot indices
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   No pitch spatial or frame-rate physics constants; screen coordinates and
 *   UI widget IDs (0x2A..0x35), palette/sprite IDs (0x08, 0x59, 0x5B, 0xC8,
 *   0xD0, 0xE0), and score-box stride 12.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800C6B48[];
extern unsigned char D_800C6B49[];
extern unsigned char D_800FF748[];
extern unsigned short D_8010A5A2;
extern unsigned char D_8010A5A8[];
extern unsigned char D_8010A5B2;

extern void func_8001CEE0(int id, int slot, int flags, int x, int y, int w, int h, int vis);
extern unsigned char *func_8001D004(unsigned int slot);
extern void func_80020AB4(void);
extern void func_80020C14(int val);
extern void func_800219F4(unsigned char *dst, int score, int arg2);
extern void func_8004ED68(int side);
extern void func_8004F25C(void);
extern void func_80072904(void);

void func_800204D8(void)
{
    unsigned char *obj;
    int vis;
    int i;
    unsigned char val;

    vis = 0;
    if ((D_8010A5B2 & 0x10) != 0) {
        vis = (B(Scratchpad, 0x2E1) != 5) ? 1 : 0;
    }
    func_8001CEE0(0x2B, 3, 0, -0x8A, -0x62, 6, 0, vis);

    func_8001CEE0(0x2C, 2, 0, -2, 0x4E, 6, 0, D_8010A5B2 & 3);

    vis = 0;
    if ((D_8010A5B2 & 8) != 0) {
        vis = (B(Scratchpad, 0x2E1) != 5) ? 1 : 0;
    }
    func_8001CEE0(0x2A, 5, 0, 0x68, -0x62, 6, 0, vis);

    vis = 0;
    if ((D_8010A5B2 & 4) != 0 || B(Scratchpad, 0x2E1) == 5) {
        vis = 1;
    }
    func_8001CEE0(0x2D, 6, 0, 8, 0, 7, 0, vis);

    obj = func_8001D004(6);
    B(obj, 0x16) = (B(Scratchpad, 0x2E1) == 5) ? 0x5B : 8;

    vis = 0;
    if ((B(D_8010A5A8, 0x0A) & 4) != 0 || B(Scratchpad, 0x2E1) == 5) {
        vis = 1;
    }
    func_8001CEE0(0x2E, 0x2C, 0, 8, 0, 6, 0, vis);

    obj = func_8001D004(0x2C);
    B(obj, 0x16) = (B(Scratchpad, 0x2E1) == 5) ? 0x5B : 0x59;

    func_8001CEE0(0x2F, 0, 0, 8, 0, 6, 0, 1);
    func_8001CEE0(0x30, 1, 0, 8, 0, 6, 0, (B(Scratchpad, 0x2E1) != 5) ? 1 : 0);

    for (i = 0; i < 2; i++) {
        obj = func_8001D004((unsigned int)i);
        if ((B(D_8010A5A8, 0x0A) & 0x20) != 0) {
            B(obj, 0x28) = 0xD0;
            B(obj, 0x26) = 0x10;
            B(obj, 0x34) = 0xE0;
            B(obj, 0x32) = 0x10;
        } else {
            B(obj, 0x28) = 0xC8;
            B(obj, 0x26) = 8;
            B(obj, 0x34) = 0xC8;
            B(obj, 0x32) = 8;
        }
        B(obj, 0x2E) = 0x5B;
        B(obj, 0x3A) = 0x5B;
    }

    vis = 0;
    if ((B(D_8010A5A8, 0x0A) & 4) != 0 || B(Scratchpad, 0x2E1) == 5) {
        vis = 1;
    }
    func_8001CEE0(0x31, 0x27, 0x60000000, 8, 0, 6, 0, vis);

    obj = func_8001D004(0x27);
    val = (B(Scratchpad, 0x2E1) == 5) ? 0x5B : 0x59;
    B(obj, 0x16) = val;
    B(obj, 0x2E) = val;

    func_8001CEE0(0x32, 7, 0, 8, 0, 6, 0, 1);
    func_8001CEE0(0x33, 8, 0, 8, 0, 6, 0, 1);

    D_8010A5A2 = 0;
    if (B(Scratchpad, 0x298) == 3) {
        unsigned char *entry = D_800FF748 + 0x708;
        for (i = 0x2D; i < 0x34; i++, entry += 0x28) {
            B(entry, 0x5DC4) = 0;
        }
    }

    func_8001CEE0(0x34, 0x25, 0, 0, 0, 6, 0, 1);
    func_8001CEE0(0x35, 0x26, 0, 0, 0, 6, 0, 1);

    obj = func_8001D004(0x25);
    B(obj, 0x0A) = D_800C6B48[B(Scratchpad, 0x2E2) * 2];

    obj = func_8001D004(0x26);
    B(obj, 0x04) = 0x28;
    B(obj, 0x0A) = D_800C6B49[B(Scratchpad, 0x2E2) * 2];

    switch (B(Scratchpad, 0x2E2)) {
    case 0:
    case 1:
    case 4:
        B(Scratchpad, 0x244 + B(Scratchpad, 0x2E0)) = 2;
        B(Scratchpad, 0x244 + (B(Scratchpad, 0x2E0) ^ 1)) = 0x0D;
        break;

    case 3:
        B(obj, 0x04) = 0x48;
        /* fall through */

    case 2: {
        unsigned int slot = B(Scratchpad, 0x2E0);
        unsigned char v = (unsigned char)(slot * 11 + 2);
        B(Scratchpad, 0x245) = v;
        B(Scratchpad, 0x244) = v;
        break;
    }
    }

    obj = func_8001D004(3);
    for (i = 0; i < 2; i++) {
        unsigned int idx = B(Scratchpad, 0x2E0) ^ (unsigned int)i;
        func_800219F4(obj + idx * 12, B(Scratchpad, 0x2DD + i), 0);
    }

    obj = func_8001D004(5);
    {
        unsigned int len = B(Scratchpad, 0x2DF);
        B(obj, 0x03) = (unsigned char)(len * 40);
        func_80020C14((int)len);
    }

    func_80072904();

    obj = func_8001D004(7);
    B(obj, 0x01) = 0;

    obj = func_8001D004(8);
    B(obj, 0x01) = 0;

    func_8004F25C();
    func_8004ED68(0);
    func_8004ED68(1);
    func_80020AB4();
}
