/*
 * func_8001EB10
 *
 * Purpose:
 *   Per-match / per-half initialization routine. Calls func_8005058C and
 *   func_8004FC54(1), sets Scratchpad+0x328 (match half/period or timer mode),
 *   clears per-team scratchpad and D_800FF748 match state arrays (96-byte
 *   0xFF-filled buffers, 22-byte zeroed player arrays, 8 team bytes), seeds
 *   random match counters in D_800FF748+0xAC50..0xAC6D via func_8001E6B8,
 *   resets display/HUD helpers (func_8001723C, func_80050888, func_8001720C),
 *   and triggers func_80017564(0xBF) when game state == 5 (match) and mode != 5.
 *
 * Arguments:
 *   void
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x298 (u8)  game state (3, 5=match) (R)
 *     +0x2A1 (u8)  (W = 0)
 *     +0x2CE (u8)  (W = 0)
 *     +0x2D1[0..1] (u8) (W = 0)
 *     +0x2D3..+0x2D6 (u8[4] per team, stride 4) (W = 0)
 *     +0x2DB[0..1] (u8) (W = 0)
 *     +0x2DD (u8)  (R)
 *     +0x2DE (u8)  (R)
 *     +0x2E1 (u8)  mode (R)
 *     +0x328 (u8)  (W)
 *     +0x331 (u8)  (W = 0)
 *     +0x333 (u8)  (W = 0)
 *     +0x334[0..1] (u8) (W = 0)
 *   Globals:
 *     D_8010A5A9 (u8) (R)
 *     D_800FF748 + 0x88A5..0x88FF, +0x8EC8..0x8ED7, +0xAC50..0xAC78 (W)
 *
 * Magic numbers:
 *   0xBF = 191 passed to func_80017564 when game state == 5 and mode != 5
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010A5A9[];

extern void func_8005058C(void);
extern void func_8004FC54(int);
extern int func_8001E6B8(int);
extern void func_8001723C(int);
extern void func_80050888(int);
extern void func_8001720C(void);
extern void func_80017564(int);

void func_8001EB10(void) {
    int team;
    int i;
    unsigned char* base = D_800FF748;

    func_8005058C();
    func_8004FC54(1);

    if (B(Scratchpad, 0x2E1) == 0 && B(Scratchpad, 0x298) == 3) {
        B(Scratchpad, 0x328) = 1;
    } else {
        B(Scratchpad, 0x328) = B(D_8010A5A9, 0);
    }

    B(Scratchpad, 0x2A1) = 0;
    B(Scratchpad, 0x2CE) = 0;

    for (team = 0; team < 2; team++) {
        int t96 = team * 0x60;
        int t22 = team * 0x16;
        unsigned char* s4 = Scratchpad + team * 4;

        B(base + team, 0x8ED6) = 0;
        B(base + team, 0x8ED4) = 0;
        B(base + team, 0x8ED2) = 0;
        B(base + team, 0x8ED0) = 0;
        B(base + team, 0x8ECE) = 0;
        B(base + team, 0x8ECC) = 0;
        B(base + team, 0x8ECA) = 0;
        B(base + team, 0x8EC8) = 0;

        B(s4, 0x2D6) = 0;
        B(s4, 0x2D5) = 0;
        B(s4, 0x2D4) = 0;
        B(s4, 0x2D3) = 0;

        B(Scratchpad + team, 0x2DB) = 0;
        B(Scratchpad + team, 0x2D1) = 0;
        B(base + team, 0xAC70) = 0;
        B(Scratchpad, 0x333) = 0;
        B(Scratchpad + team, 0x334) = 0;

        for (i = 0; i < 0x20; i++) {
            int off = t96 + i * 3;
            B(base + off, 0x88FF) = 0xFF;
            B(base + off, 0x88FE) = 0xFF;
            B(base + off, 0x88FD) = 0xFF;
        }

        for (i = 0; i < 0x16; i++) {
            B(base + t22 + i, 0x88D1) = 0;
            B(base + t22 + i, 0x88A5) = 0;
        }
    }

    B(base, 0xAC76) = 0;
    B(base, 0xAC77) = 0;
    B(base, 0xAC75) = 0;
    W(base, 0xAC78) = 0;
    B(Scratchpad, 0x331) = 0;

    B(base, 0xAC69) = (unsigned char)func_8001E6B8(0x10);
    B(base, 0xAC52) = (unsigned char)func_8001E6B8(0x10);
    B(base, 0xAC6B) = (unsigned char)func_8001E6B8(0x10);
    B(base, 0xAC50) = (unsigned char)func_8001E6B8(0x10);
    B(base, 0xAC57) = (unsigned char)func_8001E6B8(0x10);
    B(base, 0xAC66) = (unsigned char)func_8001E6B8(0x10);
    B(base, 0xAC6D) = (unsigned char)func_8001E6B8(0x10);
    B(base, 0xAC5C) = (unsigned char)func_8001E6B8(0x10);
    B(base, 0xAC5E) = (unsigned char)func_8001E6B8(0x10);
    B(base, 0xAC60) = (unsigned char)func_8001E6B8(0x10);
    B(base, 0xAC5D) = (unsigned char)func_8001E6B8(0xA);
    B(base, 0xAC68) = (unsigned char)func_8001E6B8(0x10);
    B(base, 0xAC67) = (unsigned char)func_8001E6B8(0x10);
    B(base, 0xAC5B) = (unsigned char)func_8001E6B8(0x10);
    B(base, 0xAC61) = (unsigned char)(B(Scratchpad, 0x2DE) + B(Scratchpad, 0x2DD) + func_8001E6B8(0x10));
    B(base, 0xAC62) = (unsigned char)func_8001E6B8(0x10);

    func_8001723C(0);
    func_80050888(0);
    func_8001720C();

    if (B(Scratchpad, 0x298) == 5 && B(Scratchpad, 0x2E1) != 5) {
        func_80017564(0xBF);
    }
}
