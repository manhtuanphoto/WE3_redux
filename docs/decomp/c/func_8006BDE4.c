/*
 * func_8006BDE4
 *
 * Purpose:
 *   Computes the off-ball positioning target (Scratchpad+0xF8/+0xFC) for a
 *   wing/wide or attacking AI player based on team positioning mode
 *   (Scratchpad+0x30B), predicted ball target (+0x2B8/+0x2BA), and lateral/pitch
 *   bounds. Also decrements D_800E6CB9 timer and adjusts Z toward center/ball.
 *
 * Arguments:
 *   actor - pointer to 1000-byte player actor struct
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Actor:
 *     +0x399 (u8) attack direction flag 0/1 (R)
 *     +0x3DA (u8) oscillation/timer byte cleared when func_800652EC == 0 (R/W)
 *     +0x3E1 (u8) lock/override flag (R)
 *   Scratchpad (0x1F800000):
 *     +0x00B (u8)  late Z-blend flag (R)
 *     +0x0F8 (s16) target X position (R/W)
 *     +0x0FC (s16) target Z position (R/W)
 *     +0x2B8/+0x2BA (s16) predicted ball target x/z (0x7FFF = inactive) (R)
 *     +0x2F4 (u8*) pointer to ball actor (+0x02/+0x06 s16 x/z) (R)
 *     +0x30B (u8)  team positioning mode 0..5 (R)
 *     +0x328 (u8)  team width/phase level (< 2 uses step 5, else 4) (R)
 *   Globals:
 *     D_800E6CB9 (u8) countdown timer; if non-zero, decrements and blends Z toward 0 (R/W)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x500 = 1280 raw (5.12 m) behind ball/target X
 *   0x200 = 512 raw (2.048 m) X offset in mode 4
 *   0x23E2 = 9186 raw (36.744 m) X clamp in mode 3; 0x2C08 = 11272 raw (45.088 m) in modes 4/5
 *   0x8B6 = 2230 raw (8.92 m) minimum |Z| distance from touchline/center
 *   0x840 = 2112 raw (8.448 m) threshold passed to func_800652EC
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800E6CB9[];

extern short func_8005A76C(int, int, int);
extern short func_8005A7E4(int, int, int);
extern short func_8006B7F4(unsigned char*);
extern void func_800A1484(unsigned char*, unsigned char*, unsigned char*, int);
extern int func_800652EC(unsigned char*, int);

void func_8006BDE4(unsigned char* actor) {
    int dirSign;
    unsigned char mode;
    int z;
    int absZ;

    if (B(D_800E6CB9, 0) != 0) {
        B(D_800E6CB9, 0) = (unsigned char)(B(D_800E6CB9, 0) - 1);
        S(Scratchpad, 0x0FC) = func_8005A76C(S(Scratchpad, 0x0FC), 0, 6);
    }

    dirSign = (B(actor, 0x399) == 0) ? 1 : -1;
    S(Scratchpad, 0x0F8) = func_8006B7F4(actor);

    mode = B(Scratchpad, 0x30B);
    switch (mode) {
    case 0:
        break;
    case 1: {
        unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
        S(Scratchpad, 0x0FC) = func_8005A7E4(S(Scratchpad, 0x0FC), S(ball, 0x6), 0xF);
        break;
    }
    case 2: {
        unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
        S(Scratchpad, 0x0FC) = func_8005A7E4(S(Scratchpad, 0x0FC), S(ball, 0x6), 0x14);
        break;
    }
    case 3: {
        unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
        int x;
        int absX;
        S(Scratchpad, 0x0FC) = func_8005A7E4(S(Scratchpad, 0x0FC), S(ball, 0x6), 0x1E);
        func_800A1484(actor, Scratchpad + 0x0F8, Scratchpad + 0x0FC, 0xA00);
        x = S(Scratchpad, 0x0F8);
        absX = (x >= 0) ? x : -x;
        if (absX >= 0x23E3) {
            S(Scratchpad, 0x0F8) = (x >= 0) ? 0x23E2 : -0x23E2;
        }
        if (B(actor, 0x3E1) == 0 && S(Scratchpad, 0x2B8) != 0x7FFF) {
            int curZ = S(Scratchpad, 0x0FC);
            int targetZ = S(Scratchpad, 0x2BA);
            S(Scratchpad, 0x0F8) = (signed short)(S(Scratchpad, 0x2B8) - ((signed short)dirSign * 0x500));
            S(Scratchpad, 0x0FC) = (signed short)((curZ + targetZ * 2) / 3);
        }
        break;
    }
    case 4: {
        int step = (B(Scratchpad, 0x328) < 2) ? 5 : 4;
        unsigned char* ball;
        int offset;
        int x;
        int absX;
        S(Scratchpad, 0x0FC) = func_8005A76C(S(Scratchpad, 0x0FC), 0, step);
        offset = ((int)(signed short)dirSign) << 9;
        ball = (unsigned char*)W(Scratchpad, 0x2F4);
        S(Scratchpad, 0x0F8) = (signed short)((S(Scratchpad, 0x0F8) + (S(ball, 0x2) - offset)) / 2);
        func_800A1484(actor, Scratchpad + 0x0F8, Scratchpad + 0x0FC, 0x1000);
        x = S(Scratchpad, 0x0F8);
        absX = (x >= 0) ? x : -x;
        if (absX >= 0x2C09) {
            S(Scratchpad, 0x0F8) = (x >= 0) ? 0x2C08 : -0x2C08;
        }
        break;
    }
    case 5: {
        unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
        int step = (B(Scratchpad, 0x328) < 2) ? 5 : 4;
        int x;
        int absX;
        S(Scratchpad, 0x0F8) = (signed short)(S(ball, 0x2) - ((signed short)dirSign * 0x500));
        S(Scratchpad, 0x0FC) = func_8005A76C(S(Scratchpad, 0x0FC), 0, step);
        func_800A1484(actor, Scratchpad + 0x0F8, Scratchpad + 0x0FC, 0x1000);
        x = S(Scratchpad, 0x0F8);
        absX = (x >= 0) ? x : -x;
        if (absX >= 0x2C09) {
            S(Scratchpad, 0x0F8) = (x >= 0) ? 0x2C08 : -0x2C08;
        }
        break;
    }
    }

    z = S(Scratchpad, 0x0FC);
    absZ = (z >= 0) ? z : -z;
    if (absZ < 0x8B6) {
        S(Scratchpad, 0x0FC) = (z >= 0) ? 0x8B6 : -0x8B6;
    }

    if (B(actor, 0x3E1) == 0 && B(Scratchpad, 0x30B) < 3) {
        if (func_800652EC(actor, 0x840) == 0) {
            B(actor, 0x3DA) = 0;
        }
        if ((B(actor, 0x3DA) % 10u) < 6u) {
            unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
            S(Scratchpad, 0x0FC) = func_8005A7E4(S(Scratchpad, 0x0FC), S(ball, 0x6), 0x14);
        }
    }

    if (B(Scratchpad, 0x30B) != 4 && B(Scratchpad, 0x00B) != 0) {
        unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
        S(Scratchpad, 0x0FC) = func_8005A76C(S(Scratchpad, 0x0FC), S(ball, 0x6), 3);
    }
}
