/*
 * func_8003BC84
 *
 * Purpose:
 *   Evaluates and initiates a trap/control or directional touch action (anim 0x2D)
 *   for an outfield player toward predicted ball position at frame 7
 *   (Scratchpad+0x44 / +0x74), checking angular constraints against the ball
 *   heading and position, computing step velocity (+0x3CA/+0x3CE), validating
 *   via func_8003CEE4, and updating animation, facing, and position.
 *
 * Arguments:
 *   actor - pointer to 1000-byte actor struct
 *
 * Return value:
 *   1 if action was initiated, 0 if rejected by preconditions or func_8003CEE4
 *
 * Actor / global fields read:
 *   +0x002 (s16) x, +0x006 (s16) z, +0x38D (u8) actor id,
 *   +0x392 (u8) team slot, +0x398 (u8) side, +0x3AA (u8) body heading,
 *   +0x3CA (s16) step dx, +0x3CE (s16) step dz
 *   D_8010C330[team_slot*12 + side*264] (u32) bit 4 (player trait/foot flag)
 *
 * Actor / global fields written:
 *   +0x002 (s16) x += +0x3CA, +0x006 (s16) z += +0x3CE,
 *   +0x026 (s16) rotation, +0x3AA (u8) body heading,
 *   +0x3AB (u8) target/action heading, +0x3AC (u8) mirror/foot,
 *   +0x3B1 (u8) secondary flag, +0x3CA (s16) step dx,
 *   +0x3CE (s16) step dz, +0x3D1 (u8) cleared to 0
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x036[7] (+0x44) (s16) predicted ball x at frame 7
 *   +0x066[7] (+0x74) (s16) predicted ball z at frame 7
 *   +0x0F0 (s16) scratch x, +0x0F4 (s16) scratch z
 *   +0x2A9 (u8) ball owner id
 *   +0x2F4 (ptr) ball actor (+0x002 x, +0x006 z, +0x3A4 move heading)
 *
 * Scratchpad fields written:
 *   +0x0F0 (s16) += step_dx * 8
 *   +0x0F4 (s16) += step_dz * 8
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x14400 (82944) -> sqrt = 288 raw = 1.152 m contact/reach radius^2
 *   0x50 (80) = 112.5 deg max angle diff to predicted target
 *   0x31 (49) = 68.9 deg min angle diff from ball heading
 *   0x21 (33) = 46.4 deg min angle diff from ball position angle
 *   0x10 (16) = 22.5 deg narrow cone threshold for D_8010C330 bit 4 check
 *   frame index = 7 (0.233 s), divisor = 8 frames (0.267 s), min speed = 0x20 (32) = 0.128 m/frame (3.84 m/s)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_8010C330[];

extern int func_8001E5A4(int angle, int scale);
extern int func_8001E694(int angle, int scale);
extern int func_8003CEE4(unsigned char *actor, int frame, int a2, int a3, int radiusSq);
extern int func_8003D4E8(unsigned char *actor, int a1);
extern int func_80052A68(unsigned char *actor, int frame, int radiusSq);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8005B7CC(int target, int cur, int step);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005C2AC(unsigned char *actor, int anim, int a2, int heading, int flag);
extern unsigned int func_800B1128(int valSq);

int func_8003BC84(unsigned char *actor)
{
    int s4;
    int s0;
    int s7;
    int fp;
    int angle;
    int diff;
    unsigned char sp18;
    unsigned char *ball;
    unsigned char ballHeading;
    int frameIdx;
    int divFrames;
    int dx, dz;
    unsigned int dist, stepSpeed;
    int turn;

    if ((func_80052A68(actor, 7, 0x14400) & 0xFF) != 0) {
        return 0;
    }

    s4 = func_8003D4E8(actor, 8);

    angle = func_8005B36C(S(actor, 2), S(actor, 6), S(Scratchpad, 0x44), S(Scratchpad, 0x74));
    if (((angle - s4) & 0xFF) < 0x81) {
        s0 = func_8005B36C(S(actor, 2), S(actor, 6), S(Scratchpad, 0x44), S(Scratchpad, 0x74)) - s4;
    } else {
        s0 = s4 - func_8005B36C(S(actor, 2), S(actor, 6), S(Scratchpad, 0x44), S(Scratchpad, 0x74));
    }
    if ((s0 & 0xFF) >= 0x50) {
        return 0;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    ballHeading = B(ball, 0x3A4);
    diff = (ballHeading - s4) & 0xFF;
    if (diff < 0x81) {
        if (diff < 0x31) {
            return 0;
        }
    } else {
        if (((s4 - ballHeading) & 0xFF) < 0x31) {
            return 0;
        }
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    sp18 = (unsigned char)func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));
    diff = (sp18 - s4) & 0xFF;
    if (diff < 0x81) {
        if (diff < 0x21) {
            return 0;
        }
    } else {
        if (((s4 - sp18) & 0xFF) < 0x21) {
            return 0;
        }
    }

    diff = (sp18 - s4) & 0xFF;
    if ((diff < 0x81 ? (diff < 0x10) : (((s4 - sp18) & 0xFF) < 0x10)) && ((s0 & 0xFF) < 0x10)) {
        int row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        s7 = (W(D_8010C330, row) >> 4) & 1;
        fp = s7 ^ 1;
    } else if (B(actor, 0x38D) == B(Scratchpad, 0x2A9)) {
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        angle = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));
        s7 = ((angle - s4) & 0xFF) < 0x81;
        fp = s7 ^ 1;
    } else {
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        s7 = ((s4 - B(ball, 0x3A4)) & 0xFF) < 0x81;
        fp = s7;
    }

    func_8005C2AC(actor, 0x2D, 0, s4 & 0xFF, s7);

    frameIdx = 7;
    dx = S(Scratchpad, 0xF0) - S(Scratchpad, 0x36 + frameIdx * 2);
    dz = S(Scratchpad, 0xF4) - S(Scratchpad, 0x66 + frameIdx * 2);
    dist = func_800B1128(dx * dx + dz * dz);
    divFrames = frameIdx + 1;
    stepSpeed = dist / (unsigned int)divFrames;
    if (stepSpeed < 0x21) {
        stepSpeed = 0x20;
    }

    angle = func_8005B36C(S(Scratchpad, 0xF0), S(Scratchpad, 0xF4),
                          S(Scratchpad, 0x36 + frameIdx * 2), S(Scratchpad, 0x66 + frameIdx * 2)) & 0xFF;
    S(actor, 0x3CA) = (signed short)func_8001E694(angle, stepSpeed);
    S(actor, 0x3CE) = (signed short)func_8001E5A4(angle, stepSpeed);

    S(Scratchpad, 0xF0) = (signed short)(H(Scratchpad, 0xF0) + S(actor, 0x3CA) * divFrames);
    S(Scratchpad, 0xF4) = (signed short)(H(Scratchpad, 0xF4) + S(actor, 0x3CE) * divFrames);

    if ((func_8003CEE4(actor, frameIdx, 1, 0, 0x14400) & 0xFF) == 0) {
        return 0;
    }

    func_8005BA30(actor, 0x2D, 0);
    angle = func_8005B7CC(sp18, B(actor, 0x3AA), 0x28);
    B(actor, 0x3AA) = (unsigned char)angle;
    turn = 0x1C0 - (angle & 0xFF);
    S(actor, 0x26) = (signed short)((turn % 256) << 4);

    B(actor, 0x3AB) = (unsigned char)s4;
    B(actor, 0x3AC) = (unsigned char)s7;
    B(actor, 0x3B1) = (unsigned char)fp;
    B(actor, 0x3D1) = 0;
    S(actor, 2) = (signed short)(H(actor, 2) + S(actor, 0x3CA));
    S(actor, 6) = (signed short)(H(actor, 6) + S(actor, 0x3CE));

    return 1;
}
