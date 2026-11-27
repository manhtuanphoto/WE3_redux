/*
 * func_80083F68
 *
 * Purpose:
 *   Initializes a goal kick set-piece state: resets all 22 outfield/GK player
 *   action bytes, places the ball at (+/-0x2E08, -0x20, +/-0x8B6) based on
 *   Scratchpad+0x2BC/0x2BE, configures the kicking player (Scratchpad+0x2AD)
 *   and opposing/second player (Scratchpad+0x322[side]), updates possession/team
 *   flags, positions formation/camera/HUD, and resets match timers.
 *
 * Arguments:
 *   None
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x1EC (u8)  cleared to 0 (W)
 *     +0x2AD (u8)  kicker player slot (1 or 12, % 24) (W/R)
 *     +0x2AE (u8)  second player ID (W)
 *     +0x2B8 (s16) set to 0x7FFF (W)
 *     +0x2BC (s16) > 0 places ball at +0x2E08 (taker 12), else -0x2E08 (taker 1) (R)
 *     +0x2BE (s16) > 0 places ball at +0x8B6, else -0x8B6 (R)
 *     +0x2CF (u8[2]) team possession flags (1 for kicker's team, 0 for other) (W)
 *     +0x2F4 (ptr) ball entity (+0x02 x, +0x04 y, +0x06 z, +0x396 state, +0x3A4 dir) (R/W)
 *     +0x322 (u16/u8[2]) player slot for second player per side (R)
 *     +0x334 (u16) nonzero triggers func_800729CC() (R)
 *   D_800FF748: player array base (stride 0x3E8 = 1000 bytes)
 *   D_8010C330: player attributes table (stride 0x100 per team, 0x0C per slot; bit 4 = footedness)
 *
 * Magic numbers:
 *   0x2E08 (11784), 0x8B6 (2230): goal-kick spot X and Z
 *   0x22E2 (8930): second player X placement
 *   0x2100 (8448): Z argument to func_800676C4
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define P(p,o)  (*(unsigned char**)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned int  D_8010C330[];

extern void func_80098428(void);
extern void func_800729CC(void);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern void func_80078CE4(unsigned char *actor, unsigned int angle);
extern void func_8004ED68(unsigned int slot_sel);
extern void func_800A3920(void);
extern void func_800676C4(unsigned int side, int x, int z, int mode);
extern void func_8005B864(unsigned char *actor, int arg1);
extern void func_8004ED28(int mode);
extern void func_80096050(void);
extern void func_80051810(void);
extern void func_800204D8(void);
extern void func_80089BD8(void);
extern void func_800171DC(void);

void func_80083F68(void) {
    int i;
    unsigned char *ball;
    unsigned char *kicker;
    unsigned char *other;
    unsigned char taker_slot;
    unsigned char angle;
    unsigned char side;
    unsigned int attr;
    signed short other_x;
    signed short other_z;

    func_80098428();

    if (H(Scratchpad, 0x334) != 0) {
        func_800729CC();
    }

    for (i = 0; i < 22; i++) {
        unsigned char *p = D_800FF748 + (i * 0x3E8);
        B(p, 0x77E) = 0x2D; /* player[i+1] + 0x396 */
        B(p, 0x77F) = 0;    /* player[i+1] + 0x397 */
    }

    ball = P(Scratchpad, 0x2F4);
    if (S(Scratchpad, 0x2BC) > 0) {
        S(ball, 0x02) = 0x2E08;
        B(Scratchpad, 0x2AD) = 0x0C;
    } else {
        S(ball, 0x02) = -0x2E08;
        B(Scratchpad, 0x2AD) = 1;
    }

    S(ball, 0x04) = -0x20;
    if (S(Scratchpad, 0x2BE) > 0) {
        S(ball, 0x06) = 0x8B6;
    } else {
        S(ball, 0x06) = -0x8B6;
    }

    taker_slot = (unsigned char)(B(Scratchpad, 0x2AD) % 24U);
    kicker = D_800FF748 + (taker_slot * 0x3E8);
    func_8005BA30(kicker, 0x2F, 0);

    attr = D_8010C330[(B(kicker, 0x398) * 0x100 + B(kicker, 0x392) * 0x0C) >> 2];
    B(kicker, 0x397) = 1;
    B(kicker, 0x3AC) = (unsigned char)((attr >> 4) & 1);

    ball = P(Scratchpad, 0x2F4);
    angle = (unsigned char)func_8005B36C(S(ball, 0x02), S(ball, 0x06), 0, 0);
    ball = P(Scratchpad, 0x2F4);
    B(kicker, 0x3AB) = angle;
    B(ball, 0x3A4) = angle;
    B(ball, 0x396) = 0;
    func_80078CE4(kicker, angle);

    side = B(kicker, 0x398);
    B(Scratchpad, 0x2CF + side) = 1;
    B(Scratchpad, 0x2CF + (side ^ 1)) = 0;

    func_8004ED68(0);
    func_8004ED68(1);
    func_800A3920();

    ball = P(Scratchpad, 0x2F4);
    func_800676C4(B(kicker, 0x399), S(ball, 0x02),
                  (S(ball, 0x06) >= 0) ? 0x2100 : -0x2100, 0);

    taker_slot = (unsigned char)(B(Scratchpad, 0x322 + (B(kicker, 0x399) << 1)) % 24U);
    other = D_800FF748 + (taker_slot * 0x3E8);

    ball = P(Scratchpad, 0x2F4);
    other_x = (S(ball, 0x02) >= 0) ? 0x22E2 : -0x22E2;
    S(other, 0x02) = other_x;

    other_z = (signed short)((S(ball, 0x06) * 15) / 10);
    S(other, 0x06) = other_z;

    ball = P(Scratchpad, 0x2F4);
    B(other, 0x3AA) = (unsigned char)func_8005B36C(other_x, other_z, S(ball, 0x02), S(ball, 0x06));
    W(other, 0x3A0) = 0;
    func_8005B864(other, 0);

    B(Scratchpad, 0x2AE) = B(other, 0x38D);
    func_8004ED28(5);
    func_80096050();
    func_80051810();
    func_800204D8();

    B(Scratchpad, 0x1EC) = 0;
    H(Scratchpad, 0x2B8) = 0x7FFF;
    func_80089BD8();
    func_800171DC();
}
