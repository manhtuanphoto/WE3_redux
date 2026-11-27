/*
 * func_8003A4A8
 *
 * Purpose:
 *   Evaluates and initiates an aerial/diving/jumping ball-interception action
 *   (anim 0x2E) toward predicted ball position at frame 8 (Scratchpad+0x46 /
 *   +0x5E / +0x76), capping horizontal reach speed by player stat and facing
 *   alignment, adjusting vertical velocity with func_8005C454, validating via
 *   func_8003CEE4, and updating animation and actor state flags.
 *
 * Arguments:
 *   actor - pointer to 1000-byte actor struct
 *
 * Return value:
 *   1 if action was initiated, 0 if rejected by preconditions or func_8003CEE4
 *
 * Actor / global fields read:
 *   +0x002 (s16) x, +0x006 (s16) z, +0x008 (s32) vx, +0x010 (s32) vz,
 *   +0x392 (u8) team slot, +0x398 (u8) side, +0x399 (u8) attack direction flag,
 *   +0x3A4 (u8) move heading, +0x3AA (u8) body heading,
 *   +0x3CA (s16) step dx, +0x3CC (s16) step dy, +0x3CE (s16) step dz
 *   D_8010C32F[team_slot*12 + side*264] (u8) low nibble player stat (0..15)
 *
 * Actor / global fields written:
 *   +0x026 (s16) rotation, +0x3AA (u8) body heading, +0x3AB (u8) action heading,
 *   +0x3AC (u8) mirror/side flag, +0x3B2 (u8) saved target heading (s7),
 *   +0x3CA (s16) step dx, +0x3CC (s16) step dy, +0x3CE (s16) step dz,
 *   +0x3D1 (u8) cleared to 0
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x046 (s16) predicted ball x at frame 8
 *   +0x05E (s16) predicted ball y at frame 8
 *   +0x076 (s16) predicted ball z at frame 8
 *   +0x03A[fp] (+0x46 when fp=6), +0x06A[fp] (+0x76 when fp=6)
 *   +0x0F0/+0x0F2/+0x0F4 (s16) scratch xyz
 *   +0x2F4 (ptr) ball actor (+0x002 x, +0x006 z)
 *
 * Scratchpad fields written:
 *   +0x0F0 (s16) += step_dx * 6
 *   +0x0F2 (s16) += step_dy * 6
 *   +0x0F4 (s16) += step_dz * 6
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x64000 (409600) -> sqrt = 640 raw = 2.56 m reach check radius^2
 *   0x60 (96) = 135 deg max angle difference between target and ball direction
 *   0xC0 (192) = 270 deg weight base for speed cap: ((stat + 40) * (192 - angleDiff)) / 128
 *   0x9000 (36864) -> sqrt = 192 raw = 0.768 m validation radius^2 in func_8003CEE4
 *   fp = 6 frames (0.20 s) movement step horizon
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_8010C32F[];

extern int func_8001E5A4(int angle, int scale);
extern int func_8001E694(int angle, int scale);
extern int func_8003CEE4(unsigned char *actor, int frame, int a2, int a3, int radiusSq);
extern int func_8003D4E8(unsigned char *actor, int a1);
extern int func_80052A68(unsigned char *actor, int frame, int radiusSq);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8005B7CC(int target, int cur, int step);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005C2AC(unsigned char *actor, int anim, int a2, int heading, int flag);
extern void func_8005C454(unsigned char *ptr, int a1, int a2);
extern unsigned int func_800B1128(int valSq);

int func_8003A4A8(unsigned char *actor)
{
    int s7;
    int s6;
    int s5;
    int s4;
    int fp;
    int s0;
    int angle;
    int diff;
    unsigned char *ball;
    int dx, dz;
    unsigned int dist;
    int s2;
    int row;
    int stat;
    int maxSpeed;
    int dy;
    int heading;
    int turn;

    if ((func_80052A68(actor, 8, 0x64000) & 0xFF) != 0) {
        return 0;
    }

    s7 = func_8003D4E8(actor, 9);
    angle = func_8005B36C(S(actor, 2), S(actor, 6), S(Scratchpad, 0x46), S(Scratchpad, 0x76));
    s6 = ((angle - (B(actor, 0x399) << 7)) & 0xFF) < 0x81;
    fp = 6;
    if ((s6 & 0xFF) != 0) {
        s5 = s7 + 0x40;
    } else {
        s5 = s7 - 0x40;
    }

    func_8005C2AC(actor, 0x2E, 0, s5 & 0xFF, s6);

    s4 = func_8005B36C(S(Scratchpad, 0xF0), S(Scratchpad, 0xF4),
                       S(Scratchpad, 0x3A + fp * 2), S(Scratchpad, 0x6A + fp * 2));

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    angle = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));
    if (((s4 - angle) & 0xFF) < 0x81) {
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        diff = (s4 - func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6))) & 0xFF;
    } else {
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        diff = (func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6)) - s4) & 0xFF;
    }
    if (diff >= 0x60) {
        return 0;
    }

    s0 = fp & 0xFF;
    dx = S(Scratchpad, 0xF0) + SW(actor, 8) - S(Scratchpad, 0x3A + s0 * 2);
    dz = S(Scratchpad, 0xF4) + SW(actor, 0x10) - S(Scratchpad, 0x6A + s0 * 2);
    dist = func_800B1128(dx * dx + dz * dz);
    s2 = (int)(dist / (unsigned int)s0);

    row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
    stat = B(D_8010C32F, row) & 0xF;
    diff = (B(actor, 0x3A4) - s5) & 0xFF;
    if (diff >= 0x81) {
        diff = (s5 - B(actor, 0x3A4)) & 0xFF;
    }
    maxSpeed = ((stat + 0x28) * (0xC0 - diff)) / 128;
    if (maxSpeed < s2) {
        s2 = maxSpeed;
    }

    s0 = s4 & 0xFF;
    S(actor, 0x3CA) = (signed short)func_8001E694(s0, s2);
    S(actor, 0x3CE) = (signed short)func_8001E5A4(s0, s2);

    s0 = fp & 0xFF;
    dy = S(Scratchpad, 0x5E) - S(Scratchpad, 0xF2);
    S(actor, 0x3CC) = (signed short)(dy / s0);

    row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
    func_8005C454(actor + 0x3CC, 0x12, -8 - ((B(D_8010C32F, row) & 0xF) >> 1));

    S(Scratchpad, 0xF0) = (signed short)(H(Scratchpad, 0xF0) + S(actor, 0x3CA) * s0);
    S(Scratchpad, 0xF2) = (signed short)(H(Scratchpad, 0xF2) + S(actor, 0x3CC) * s0);
    S(Scratchpad, 0xF4) = (signed short)(H(Scratchpad, 0xF4) + S(actor, 0x3CE) * s0);

    if ((func_8003CEE4(actor, 8, 1, 1, 0x9000) & 0xFF) == 0) {
        return 0;
    }

    func_8005BA30(actor, 0x2E, 0);
    heading = func_8005B7CC(s5 & 0xFF, B(actor, 0x3AA), 0xC);
    B(actor, 0x3AA) = (unsigned char)heading;
    turn = 0x1C0 - (heading & 0xFF);
    S(actor, 0x26) = (signed short)((turn % 256) << 4);

    B(actor, 0x3B2) = (unsigned char)s7;
    B(actor, 0x3AB) = (unsigned char)s5;
    B(actor, 0x3D1) = 0;
    B(actor, 0x3AC) = (unsigned char)s6;

    return 1;
}
