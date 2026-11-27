/*
 * func_8007A774
 *
 * Purpose:
 *   Initializes a corner kick set piece: clamps the ball to the corner flag
 *   (x clamped to +-0x32A0, z = +-0x2300, y = 0), sets ball state = 3 and
 *   possession flags, transitions all 22 field players into set-piece state 0x2C,
 *   places the corner taker at the ball facing the penalty area, positions a
 *   nearby teammate at 0xA00 (10.24 m) and an opponent at 0x1000 (16.384 m) along
 *   the corner heading, and runs formation/camera/HUD reset helpers.
 *
 * Arguments:
 *   void
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x1E8 (u8)  set to 1 (W)
 *     +0x1EC (u8)  cleared to 0 (W)
 *     +0x294 (u16) cleared to 0 (W)
 *     +0x2A8 (u8)  last-touch actor (W = kicker id)
 *     +0x2A9 (u8)  ball owner (W = kicker id)
 *     +0x2AD (u8)  designated corner-kick taker actor id (R)
 *     +0x2B4 (u16), +0x2B8 (u16) predicted target sentinels (W = 0x7FFF)
 *     +0x2BC (s16), +0x2BE (s16) out-of-bounds ball X, Z (R)
 *     +0x2CF (u8[2]) per-side possession flags (W)
 *     +0x2E1 (u8)  match mode (R)
 *     +0x2F4 (u8*) pointer to ball actor (R)
 *     +0x31E..+0x323 (u8[]) set-piece player role/slot IDs (R/W)
 *     +0x334 (u16) non-zero triggers func_800729CC (R)
 *   Actor fields:
 *     Ball: +0x02/+0x04/+0x06 (s16) xyz, +0x396 (u8 = 3), +0x3B1 (u8 = 4)
 *     All 22 players: +0x396 (u8 = 0x2C), +0x397 (u8 = 0), +0x39A (u8) skip flag
 *     Kicker: +0x02/+0x06, +0x397 (5), +0x3A0 (0), +0x3AA/+0x3AB (heading)
 *     Teammate/Opponent: +0x02/+0x06 positioned relative to kicker
 *   Globals:
 *     D_800FF748: actor array + overlay slots (+0x61D0, stride 0x28 x 10) + 0x88A4
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x32A0 = 12960 raw (51.84 m) goal-line X clamp
 *   0x2300 = 8960 raw (35.84 m) corner flag Z
 *   0x28F0 = 10480 raw (41.92 m) penalty-area X aim target
 *   0x0A00 = 2560 raw (10.24 m) teammate offset; 0x1000 = 4096 raw (16.384 m) opponent offset
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];

extern void func_800729CC(void);
extern void func_800A4630(void);
extern void func_8005BA30(unsigned char*, int, int);
extern int func_8005B36C(int, int, int, int);
extern void func_8005B864(unsigned char*, int);
extern void func_800676C4(int, int, int, int);
extern int func_8001E694(int, int);
extern int func_8001E5A4(int, int);
extern void func_800512B8(unsigned char*, int);
extern void func_8004ED68(int);
extern void func_80096050(void);
extern void func_80051810(void);
extern void func_80089BD8(void);
extern void func_800171DC(void);

void func_8007A774(void) {
    unsigned char* ball;
    unsigned int heading;
    unsigned char* kicker;
    unsigned char* player;
    unsigned char* slotPtr;
    unsigned char* otherActor;
    unsigned char atkDir;
    int i;

    ball = (unsigned char*)W(Scratchpad, 0x2F4);
    B(Scratchpad, 0x1E8) = 1;
    H(ball, 2) = H(Scratchpad, 0x2BC);

    ball = (unsigned char*)W(Scratchpad, 0x2F4);
    if (S(ball, 2) >= 0x32A1) {
        S(ball, 2) = 0x32A0;
        ball = (unsigned char*)W(Scratchpad, 0x2F4);
    }
    if (S(ball, 2) < -0x32A0) {
        S(ball, 2) = -0x32A0;
    }

    ball = (unsigned char*)W(Scratchpad, 0x2F4);
    S(ball, 4) = 0;

    if (S(Scratchpad, 0x2BE) >= 0) {
        heading = 0xC0;
        S(W(Scratchpad, 0x2F4), 6) = 0x2300;
    } else {
        heading = 0x40;
        S(W(Scratchpad, 0x2F4), 6) = -0x2300;
    }

    B(W(Scratchpad, 0x2F4), 0x396) = 3;
    H(Scratchpad, 0x2B4) = 0x7FFF;
    H(Scratchpad, 0x2B8) = 0x7FFF;
    if (H(Scratchpad, 0x334) != 0) {
        func_800729CC();
    }

    kicker = D_800FF748 + (B(Scratchpad, 0x2AD) % 24u) * 1000;
    B(Scratchpad, 0x2CF + B(kicker, 0x398)) = 0;
    B(Scratchpad, 0x2CF + (B(kicker, 0x398) ^ 1)) = 1;
    func_800A4630();

    player = D_800FF748 + 0x3E8;
    for (i = 0; i < 22; i++) {
        if (B(Scratchpad, 0x2E1) != 5 || B(player, 0x39A) == 0) {
            B(player, 0x396) = 0x2C;
            B(player, 0x397) = 0;
        }
        player += 0x3E8;
    }

    kicker = D_800FF748 + (B(Scratchpad, 0x2AD) % 24u) * 1000;
    func_8005BA30(kicker, 0x1D, 0);
    B(W(Scratchpad, 0x2F4), 0x3B1) = 4;
    B(kicker, 0x397) = 5;
    H(kicker, 2) = H(W(Scratchpad, 0x2F4), 2);
    H(kicker, 6) = H(W(Scratchpad, 0x2F4), 6);
    W(kicker, 0x3A0) = 0;
    B(kicker, 0x3AB) = (unsigned char)heading;

    if (B(kicker, 0x39A) == 0) {
        int aimX = (B(kicker, 0x399) != 0) ? -0x28F0 : 0x28F0;
        heading = (unsigned int)func_8005B36C(S(kicker, 2), S(kicker, 6), aimX, 0);
    }

    B(kicker, 0x3AA) = (unsigned char)heading;
    func_8005B864(kicker, 0);

    B(Scratchpad, 0x2A9) = B(kicker, 0x38D);
    B(Scratchpad, 0x2A8) = B(kicker, 0x38D);

    ball = (unsigned char*)W(Scratchpad, 0x2F4);
    func_800676C4(0, S(ball, 2), S(ball, 6), 0);
    ball = (unsigned char*)W(Scratchpad, 0x2F4);
    func_800676C4(1, S(ball, 2), S(ball, 6), 0);

    atkDir = B(kicker, 0x399);
    B(Scratchpad, 0x31E + atkDir * 2) = B(Scratchpad, 0x322 + atkDir * 2);
    B(Scratchpad, 0x31F + atkDir * 2) = B(Scratchpad, 0x323 + atkDir * 2);
    B(Scratchpad, 0x31E + (atkDir ^ 1) * 2) = B(Scratchpad, 0x322 + (atkDir ^ 1) * 2);

    slotPtr = Scratchpad + 0x322 + B(kicker, 0x399) * 2;
    i = 0;
    do {
        otherActor = D_800FF748 + (B(slotPtr, 0) % 24u) * 1000;
        i++;
        if (otherActor != kicker) {
            break;
        }
        slotPtr++;
    } while (i < 2);

    heading &= 0xFF;
    S(otherActor, 2) = (signed short)(S(kicker, 2) + func_8001E694((int)heading, 0xA00));
    S(otherActor, 6) = (signed short)(S(kicker, 6) + func_8001E5A4((int)heading, 0xA00));

    otherActor = D_800FF748 + (B(Scratchpad, 0x322 + (B(kicker, 0x399) ^ 1) * 2) % 24u) * 1000;
    if (B(Scratchpad, 0x2E1) != 5 || B(otherActor, 0x39A) == 0) {
        S(otherActor, 2) = (signed short)(S(kicker, 2) + func_8001E694((int)heading, 0x1000));
        S(otherActor, 6) = (signed short)(S(kicker, 6) + func_8001E5A4((int)heading, 0x1000));
    }

    func_800512B8(kicker, 6);

    slotPtr = D_800FF748 + 0x61D0;
    for (i = 9; i >= 0; i--) {
        B(slotPtr, 4) = 0;
        slotPtr += 0x28;
    }

    func_8004ED68(0);
    func_8004ED68(1);
    func_80096050();
    func_80051810();
    B(Scratchpad, 0x1EC) = 0;
    func_80089BD8();
    B(D_800FF748, 0x88A4) = 1;
    H(Scratchpad, 0x294) = 0;
    func_800171DC();
}
