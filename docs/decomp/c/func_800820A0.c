/*
 * func_800820A0
 *
 * Purpose:
 *   Sets up a penalty shootout kick turn: every 10 kicks (D_8010A400 % 10 == 0)
 *   resets 10 entries in the buffer returned by func_8001D004(0x3F) to 0x5B;
 *   selects the next active kicker for team (D_8010A400 & 1) from the 11-slot
 *   kick-order table at D_800FF748+0xABAA (advancing D_800FB5AA[team]), sets up
 *   the opposing goalkeeper at X = -0x32E0, updates scoreboard/HUD coordinates,
 *   triggers penalty SFX 0x51A, and initializes the round state.
 *
 * Arguments:
 *   None
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x294 (s16) cleared to 0 (W)
 *     +0x2AA (u8)  set to 0xFF (W)
 *     +0x2AD (u8)  kicker player ID (W)
 *     +0x2AE (u8)  goalkeeper player ID (W)
 *   D_8010A400 (u8): penalty shootout kick counter; bit 0 = kicking team (R)
 *   D_800FB5AA[2] (u8): per-team next index in 11-player shootout order (R/W)
 *   D_800FF748: player array base (stride 0x3E8); +0xABAA holds kicking order;
 *               +0x64D8/64DA/6500/6502 (u16) shifted by (-0x48, -0x40)
 *   D_8010C330: player attributes table (bit 4 = footedness -> actor+0x3AC)
 *   D_800E6D60, D_800E7DE9 (u8): set to 0xFF (W)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_8010A400;
extern unsigned char D_800FB5AA[];
extern unsigned char D_800FF748[];
extern unsigned int  D_8010C330[];
extern unsigned char D_800E6D60;
extern unsigned char D_800E7DE9;

extern void func_80098428(void);
extern unsigned char *func_8001D004(int arg0);
extern void func_80081DEC(void);
extern void func_80078CE4(unsigned char *actor, unsigned int angle);
extern void func_800512B8(unsigned char *actor, unsigned int cmd);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005C8D0(unsigned char *actor, int arg1, int arg2);
extern void func_8005B864(unsigned char *actor, int arg1);
extern void func_8001CEE0(int a0, int a1, unsigned int a2, int a3,
                           int a4, int a5, int a6, int a7);
extern void func_80051810(void);
extern void func_800AB620(unsigned int sfx);
extern void func_800171DC(void);

void func_800820A0(void) {
    unsigned char team;
    int team_base;
    int wrap_base;
    unsigned char order_idx;
    unsigned char *order_tbl;
    unsigned char *kicker;
    unsigned char *gk;
    unsigned int attr;
    int i;

    func_80098428();

    if ((D_8010A400 % 10U) == 0) {
        unsigned char *buf = func_8001D004(0x3F) + 0x6C;
        for (i = 0; i < 10; i++) {
            B(buf, 0x0A) = 0x5B;
            buf -= 0x0C;
        }
    }

    func_80081DEC();

    team = (unsigned char)(D_8010A400 & 1);
    team_base = team * 11;
    wrap_base = team_base - ((team_base / 24) * 24);
    order_idx = (unsigned char)(D_800FB5AA[team] % 11U);
    order_tbl = D_800FF748 + 0xABAA + (team_base * 2);

    while (1) {
        int slot = order_tbl[order_idx] + wrap_base;
        kicker = D_800FF748 + (slot * 0x3E8);
        if (B(kicker, 0x00) != 0) {
            break;
        }
        order_idx = (unsigned char)((order_idx + 1) % 11);
    }

    D_800FB5AA[team] = (unsigned char)(order_idx + 1);
    B(kicker, 0x397) = 1;
    B(kicker, 0x3D3) = 1;
    B(Scratchpad, 0x2AD) = B(kicker, 0x38D);

    attr = D_8010C330[(B(kicker, 0x398) * 0x100 + B(kicker, 0x392) * 0x0C) >> 2];
    B(kicker, 0x3AC) = (unsigned char)((attr >> 4) & 1);

    func_80078CE4(kicker, 0x80);
    func_800512B8(kicker, 5);
    func_8005BA30(kicker, 0x2F, 0);
    func_8005C8D0(kicker, 0, 0x2C);

    if ((D_8010A400 & 1) != 0) {
        gk = D_800FF748 + 0x3E8;  /* player 1 */
    } else {
        gk = D_800FF748 + 0x2EE0; /* player 12 */
    }

    func_8005BA30(gk, 0x5A, 0);
    B(gk, 0x397) = 0x88;
    B(Scratchpad, 0x2AE) = B(gk, 0x38D);
    W(gk, 0x3A0) = 0;
    S(gk, 0x02) = -0x32E0;
    S(gk, 0x06) = 0;
    S(gk, 0x04) = 0;
    B(gk, 0x3AA) = 0;
    func_8005B864(gk, 0);

    H(D_800FF748, 0x64D8) = (unsigned short)(H(D_800FF748, 0x64D8) - 0x48);
    H(D_800FF748, 0x64DA) = (unsigned short)(H(D_800FF748, 0x64DA) - 0x40);
    H(D_800FF748, 0x6500) = (unsigned short)(H(D_800FF748, 0x6500) - 0x48);
    H(D_800FF748, 0x6502) = (unsigned short)(H(D_800FF748, 0x6502) - 0x40);

    func_8001CEE0(0x32, 0x3F, 0x60000000U, 0x10, 0x10, 6, 0, 1);

    H(Scratchpad, 0x294) = 0;
    D_800E6D60 = 0xFF;
    D_800E7DE9 = 0xFF;
    B(Scratchpad, 0x2AA) = 0xFF;

    func_80051810();
    func_800AB620(0x51A);
    func_800171DC();
}
