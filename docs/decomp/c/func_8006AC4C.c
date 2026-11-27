/*
 * func_8006AC4C
 *
 * Purpose:
 *   Computes the off-ball positioning target (Scratchpad+0xF8/+0xFC) for an
 *   AI player based on team mentality/tactics (Scratchpad+0x2FF/0x30B),
 *   ball position, offside/formation line bounds (Scratchpad+0x30C/0x30E),
 *   and pressing/defensive state in D_8010C1D8.
 *
 * Arguments:
 *   actor - pointer to 1000-byte player actor struct
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Actor:
 *     +0x398 (u8) side 0/1 (R)
 *     +0x399 (u8) attack direction flag 0/1 (R)
 *   Scratchpad (0x1F800000):
 *     +0x009 (u8)  phase/tactic shift enable flag (R)
 *     +0x0F8 (s16) target X position (R/W)
 *     +0x0FC (s16) target Z position (R/W)
 *     +0x2F4 (u8*) pointer to ball actor (+0x02/+0x06 s16 x/z) (R)
 *     +0x2FF (u8[2]) per-side mentality/aggressiveness 0..2 (R)
 *     +0x30B (u8)  team positioning mode 0..5 (R)
 *     +0x30C/+0x30E (s16[2]) defense/offside X bounds (R)
 *     +0x314 (s32[2]) goal/anchor X coordinate per attack direction (R)
 *   Globals:
 *     D_8010C1D8: match/team AI state (+0x0D[side] u8 tactic mode, +0x3A u16 flag) (R)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x100 = 256 raw (~1.024 m) X margin outside defense/offside line
 *   0x1000 / 0xC00 / 0xA00 = 4096 / 3072 / 2560 interpolation weights for func_800A1484
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_8010C1D8[];

extern short func_8005A7E4(int, int, int);
extern short func_8005A76C(int, int, int);
extern int func_8009FE1C(unsigned char*);
extern void func_800A1484(unsigned char*, unsigned char*, unsigned char*, int);

void func_8006AC4C(unsigned char* actor) {
    unsigned char* teamState = D_8010C1D8;
    unsigned int atkDir = B(actor, 0x399);
    unsigned char mode;

    if (B(Scratchpad, 0x009) != 0) {
        unsigned char mentality = B(Scratchpad + B(actor, 0x398), 0x2FF);
        switch (mentality) {
        case 0:
            S(Scratchpad, 0x0F8) = func_8005A7E4(S(Scratchpad, 0x0F8), SW(Scratchpad, 0x314 + atkDir * 4u), 0x19);
            break;
        case 1:
            S(Scratchpad, 0x0F8) = func_8005A7E4(S(Scratchpad, 0x0F8), SW(Scratchpad, 0x314 + atkDir * 4u), 0x12);
            break;
        case 2:
            S(Scratchpad, 0x0F8) = func_8005A7E4(S(Scratchpad, 0x0F8), SW(Scratchpad, 0x314 + atkDir * 4u), 0xA);
            break;
        }
    }

    mode = B(Scratchpad, 0x30B);
    switch (mode) {
    case 0: {
        unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
        int dir = B(actor, 0x399);
        int midX;
        S(Scratchpad, 0x0FC) = func_8005A76C(S(Scratchpad, 0x0FC), S(ball, 0x6), 5);
        midX = (SW(Scratchpad, 0x314 + (unsigned int)dir * 4u) + S(Scratchpad, 0x30C + ((unsigned int)(dir ^ 1) * 2u))) / 2;
        S(Scratchpad, 0x0F8) = (signed short)((S(Scratchpad, 0x0F8) + midX) / 2);
        break;
    }
    case 1: {
        unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
        S(Scratchpad, 0x0FC) = func_8005A76C(S(Scratchpad, 0x0FC), S(ball, 0x6), 4);
        if (func_8009FE1C(actor) != 0) {
            func_800A1484(actor, Scratchpad + 0x0F8, Scratchpad + 0x0FC, 0x1000);
        }
        break;
    }
    case 2: {
        unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
        S(Scratchpad, 0x0FC) = func_8005A76C(S(Scratchpad, 0x0FC), S(ball, 0x6), 3);
        if (func_8009FE1C(actor) != 0) {
            func_800A1484(actor, Scratchpad + 0x0F8, Scratchpad + 0x0FC, 0x1000);
            S(Scratchpad, 0x0F8) = func_8005A76C(S(Scratchpad, 0x0F8), SW(Scratchpad, 0x314 + (unsigned int)B(actor, 0x399) * 4u), 1);
        }
        break;
    }
    case 3:
        S(Scratchpad, 0x0FC) = func_8005A76C(S(Scratchpad, 0x0FC), 0, 2);
        func_800A1484(actor, Scratchpad + 0x0F8, Scratchpad + 0x0FC, 0xC00);
        break;
    case 4:
    case 5:
        S(Scratchpad, 0x0FC) = func_8005A76C(S(Scratchpad, 0x0FC), 0, 2);
        func_800A1484(actor, Scratchpad + 0x0F8, Scratchpad + 0x0FC, 0xA00);
        break;
    }

    if (B(actor, 0x399) == 0) {
        int lim = S(Scratchpad, 0x30E) + 0x100;
        if (S(Scratchpad, 0x0F8) > lim) {
            S(Scratchpad, 0x0F8) = (signed short)lim;
        }
    } else {
        int lim = S(Scratchpad, 0x30C) - 0x100;
        if (S(Scratchpad, 0x0F8) < lim) {
            S(Scratchpad, 0x0F8) = (signed short)lim;
        }
    }

    if (B(teamState + B(actor, 0x398), 0xD) == 4) {
        int dir = B(actor, 0x399);
        unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
        if ((dir == 0 && S(ball, 0x2) > 0) || (dir == 1 && S(ball, 0x2) < 0)) {
            S(Scratchpad, 0x0F8) = func_8005A76C(S(Scratchpad, 0x0F8), SW(Scratchpad, 0x314 + (unsigned int)B(actor, 0x399) * 4u), 8);
        }
    }

    if (H(teamState, 0x3A) != 0) {
        unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
        S(Scratchpad, 0x0F8) = func_8005A76C(S(Scratchpad, 0x0F8), SW(Scratchpad, 0x314 + (unsigned int)B(actor, 0x399) * 4u), 9);
        S(Scratchpad, 0x0FC) = func_8005A76C(S(Scratchpad, 0x0FC), S(ball, 0x6), 3);
    }
}
