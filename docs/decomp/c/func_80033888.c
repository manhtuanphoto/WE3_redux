/*
 * func_80033888
 *
 * Purpose:
 *   Executes a player kick/clearance/shot using stat data in D_8010C32E /
 *   D_8010C330. Computes kick heading (actor+0x3C4 + rand(8)), randomized base
 *   power and lift, angular accuracy penalty relative to +0x3C2, and non-
 *   preferred foot penalty (-32/256 when +0x3AC != D_8010C330 bit 4). Fires
 *   func_8004B0F0, configures ball spin/state/position from SCRATCH+0x36/0x4E/0x66,
 *   updates chasers (func_80049A94), plays SFX 0x507, checks offside
 *   (func_80089D00), sets SCRATCH+0x2B0 = 2, and calls func_80056A98.
 *
 * Arguments:
 *   actor (unsigned char *): pointer to 1000-byte player actor struct
 *
 * Return value:
 *   void
 *
 * Scratchpad / Actor / Global fields read & written:
 *   SCRATCH 0x036/0x04E/0x066 (s16, R): ball predicted frame-0 xyz (copied to ball)
 *   SCRATCH 0x2B0 (u32, W): ball event code set to 2
 *   SCRATCH 0x2B4/0x2B6 (s16, R): target/landing x/z
 *   SCRATCH 0x2F4 (ptr, R): ball actor pointer
 *   D_8010C32E[side*264 + slot*12] (u16, R): low 4 bits = player stat (0xB - stat passed to func_8005C7A0)
 *   D_8010C330[side*264 + slot*12] (u32, R): bit 4 = preferred foot
 *   Actor +0x392 (u8, R): roster slot
 *   Actor +0x398 (u8, R): side (0/1)
 *   Actor +0x3AA (u8, R): body heading
 *   Actor +0x3AC (u8, R): striking foot (0/1)
 *   Actor +0x3C2 (u8, R): aim/desired heading
 *   Actor +0x3C4 (u8, R): base kick heading
 *   Actor +0x3D1 (u8, W): cleared to 0
 *   Ball +0x02/+0x04/+0x06 (s16, W): snapped to SCRATCH+0x36/0x4E/0x66
 *   Ball +0x396 (u8, W): set to 4
 *   Ball +0x3B2 (u8, W): set to 8
 *   Ball +0x3BC (s16, W): set to 1
 *   Ball +0x3C8/+0x3CA (s16, W): spin/curve parameters
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned char D_8010C32E[];
extern unsigned char D_8010C330[];

extern void         func_8005C7A0(int val);
extern unsigned int func_8001E6B8(unsigned int n);
extern void         func_8004B0F0(unsigned char *actor, unsigned int heading, unsigned int power, int lift);
extern void         func_8004B278(unsigned char *actor);
extern void         func_80049A94(unsigned char *actor, unsigned int mode, unsigned int flag2, int x, int z);
extern void         func_800AB620(unsigned int sfx_id);
extern void         func_80089D00(unsigned char *actor, unsigned int target_id, int x, int z);
extern void         func_80056A98(unsigned char *actor);

void func_80033888(unsigned char *actor) {
    unsigned int stat_off;
    unsigned int r;
    unsigned int kick_h;
    unsigned int base_power;
    int lift;
    unsigned int aim_h;
    unsigned int err;
    int scale;
    unsigned int pref_foot;
    unsigned int power;
    unsigned char *ball;
    unsigned int r2;
    unsigned int body_diff;
    int spin_pen;

    stat_off = (unsigned int)B(actor, 0x392) * 12u + (unsigned int)B(actor, 0x398) * 264u;
    func_8005C7A0((signed short)(0x0B - (int)(H(D_8010C32E, stat_off) & 0x0Fu)));

    r = func_8001E6B8(8);
    kick_h = (unsigned int)B(actor, 0x3C4) + r;

    switch (r) {
    case 0:
        base_power = 0xB0;
        lift = (int)func_8001E6B8(4) + 0x24;
        break;
    case 1:
    case 3:
        base_power = 0xB4;
        lift = (int)func_8001E6B8(4) + 0x20;
        break;
    case 2:
        base_power = 0xB8;
        lift = (int)func_8001E6B8(4) + 0x1C;
        break;
    default:
        base_power = 0xB8;
        lift = (int)func_8001E6B8(4) + 0x22;
        break;
    }

    aim_h = B(actor, 0x3C2);
    err = (kick_h - aim_h) & 0xFFu;
    if (err < 0x81u) {
        scale = 0x100 - (int)(err >> 2);
    } else {
        scale = 0x100 - (int)(((aim_h - kick_h) & 0xFFu) >> 2);
    }

    stat_off = (unsigned int)B(actor, 0x392) * 12u + (unsigned int)B(actor, 0x398) * 264u;
    pref_foot = (W(D_8010C330, stat_off) >> 4) & 1u;
    if (B(actor, 0x3AC) != pref_foot) {
        scale -= 0x20;
    }

    base_power -= func_8001E6B8(base_power >> 2);
    power = (unsigned int)(((int)base_power * scale) >> 8);
    func_8004B0F0(actor, kick_h & 0xFFu, power, lift);

    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    S(ball, 0x3BC) = 1;
    B(ball, 0x3B2) = 8;
    S(ball, 0x3CA) = (signed short)(((kick_h - B(actor, 0x3AA)) & 0xFFu) < 0x81u);

    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    r2 = func_8001E6B8(2);
    body_diff = (B(actor, 0x3AA) - kick_h) & 0xFFu;
    if (body_diff < 0x81u) {
        spin_pen = (int)body_diff >> 6;
    } else {
        spin_pen = (int)((kick_h - B(actor, 0x3AA)) & 0xFFu) >> 6;
    }
    S(ball, 0x3C8) = (signed short)((0x0A - (int)r2) - spin_pen);

    B(actor, 0x3D1) = 0;

    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    H(ball, 0x02) = H(SCRATCH, 0x036);
    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    H(ball, 0x04) = H(SCRATCH, 0x04E);
    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    H(ball, 0x06) = H(SCRATCH, 0x066);
    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    B(ball, 0x396) = 4;

    func_8004B278(actor);
    func_80049A94(actor, 1, 1, S(SCRATCH, 0x2B4), S(SCRATCH, 0x2B6));
    func_800AB620(0x507);
    func_80089D00(actor, 0, S(SCRATCH, 0x2B4), S(SCRATCH, 0x2B6));
    W(SCRATCH, 0x2B0) = 2;
    func_80056A98(actor);
}
