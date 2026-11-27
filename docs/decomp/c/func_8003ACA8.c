/*
 * func_8003ACA8
 *
 * Purpose:
 *   Attempts a diving/lunging header or volley animation (anim 0x31) toward the
 *   predicted ball position 5-6 frames ahead. Checks stamina/eligibility,
 *   target angle alignment, body-heading tolerance, vertical reach, and
 *   collision/space clearance; if valid, initiates the anim and advances actor.
 *
 * Arguments:
 *   actor - pointer to 1000-byte player actor struct
 *
 * Return value:
 *   1 if the diving/lunging action was started, 0 otherwise.
 *
 * Reads / Writes:
 *   Actor:
 *     +0x002/+0x004/+0x006 (s16) x/y/z position (R/W)
 *     +0x026 (u16) model rotation (W)
 *     +0x392 (u8)  team slot 0..10 (R)
 *     +0x398 (u8)  side 0/1 (R)
 *     +0x3AA (u8)  body heading (R/W)
 *     +0x3AB (u8)  target heading (W)
 *     +0x3AC (u8)  mirror/foot flag (W)
 *     +0x3B2 (u8)  action/target flag from func_8003D4E8 (W)
 *     +0x3CA/+0x3CC/+0x3CE (s16) per-frame step velocity x/y/z (R/W)
 *     +0x3D1 (u8)  cleared to 0 (W)
 *   Scratchpad (0x1F800000):
 *     +0x036.. (s16[]) predicted ball X table (index 5 = +0x40) (R)
 *     +0x04E.. (s16[]) predicted ball Y table (index 5 = +0x58) (R)
 *     +0x066.. (s16[]) predicted ball Z table (index 5 = +0x70) (R)
 *     +0x0F0/+0x0F2/+0x0F4 (s16) contact/reach point x/y/z (R/W)
 *   Globals:
 *     D_8010C32F: player stat nibble table (R)
 *     D_8010C330: player flags table (bit 4 = preferred foot) (R)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x24000 = 147456 cost/threshold for func_80052A68
 *   0x14400 = 82944 parameter for func_8003CEE4
 *   0x20 = 32 (~45 deg) angle diff limit between aim and ball heading
 *   0x40 = 64 (~90 deg) angle diff limit from current body heading
 *   frameIdx = 5 -> 6 frames (~0.20 s) lookahead; clamp step Y to [-8, 16]
 *   vertical reach tolerance: (contactY - ballY + 255) < 447 ([-255, +191] raw)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_8010C32F[];
extern unsigned char D_8010C330[];

extern unsigned int func_80052A68(unsigned char*, int, int);
extern unsigned int func_8003D4E8(unsigned char*, int);
extern unsigned int func_8005B36C(int, int, int, int);
extern void func_8005C2AC(unsigned char*, unsigned int, unsigned int, unsigned int, unsigned int);
extern unsigned int func_800B1128(int);
extern int func_8001E694(unsigned int, unsigned int);
extern int func_8001E5A4(unsigned int, unsigned int);
extern void func_8005C454(unsigned char*, int, int);
extern unsigned int func_8003CEE4(unsigned char*, int, int, int, int);
extern void func_8005BA30(unsigned char*, int, int);
extern unsigned int func_8005B7CC(unsigned int, unsigned int, int);

int func_8003ACA8(unsigned char* actor) {
    unsigned int aimHeading;
    unsigned int ballHeading;
    unsigned int diff;
    unsigned int bodyHeading;
    unsigned int row;
    unsigned int footFlag;
    int frameIdx;
    int frames;
    unsigned int lungeHeading;
    int dx;
    int dz;
    unsigned int stepSpeed;
    unsigned int statHalf;
    int dy;
    unsigned int newHeading;
    int rot;

    if ((func_80052A68(actor, 5, 0x24000) & 0xFFu) != 0u) {
        return 0;
    }

    aimHeading = func_8003D4E8(actor, 6);
    ballHeading = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(Scratchpad, 0x40), S(Scratchpad, 0x70));

    diff = (aimHeading - ballHeading) & 0xFFu;
    if (diff < 0x81u) {
        if (diff >= 0x20u) {
            return 0;
        }
    } else {
        if (((ballHeading - aimHeading) & 0xFFu) >= 0x20u) {
            return 0;
        }
    }

    bodyHeading = B(actor, 0x3AA);
    diff = (ballHeading - bodyHeading) & 0xFFu;
    if (diff < 0x81u) {
        if (diff >= 0x40u) {
            return 0;
        }
    } else {
        if (((bodyHeading - ballHeading) & 0xFFu) >= 0x40u) {
            return 0;
        }
    }

    B(actor, 0x3AB) = (unsigned char)ballHeading;
    row = (unsigned int)B(actor, 0x392) * 12u + (unsigned int)B(actor, 0x398) * 264u;
    footFlag = (W(D_8010C330, row) >> 4) & 1u;
    func_8005C2AC(actor, 0x31, 0, ballHeading & 0xFFu, footFlag);

    frameIdx = 5;
    lungeHeading = func_8005B36C(
        S(Scratchpad, 0x0F0),
        S(Scratchpad, 0x0F4),
        S(Scratchpad + frameIdx * 2, 0x36),
        S(Scratchpad + frameIdx * 2, 0x66)
    );
    dx = S(Scratchpad, 0x0F0) - S(Scratchpad + frameIdx * 2, 0x36);
    dz = S(Scratchpad, 0x0F4) - S(Scratchpad + frameIdx * 2, 0x66);
    frames = frameIdx + 1;
    stepSpeed = func_800B1128(dx * dx + dz * dz) / (unsigned int)frames;

    row = (unsigned int)B(actor, 0x392) * 12u + (unsigned int)B(actor, 0x398) * 264u;
    statHalf = (unsigned int)((int)(B(D_8010C32F, row) & 0xFu) >> 1);
    if (stepSpeed >= statHalf + 0x34u) {
        stepSpeed = statHalf + 0x22u;
    }

    lungeHeading &= 0xFFu;
    S(actor, 0x3CA) = (signed short)func_8001E694(lungeHeading, stepSpeed);

    dy = S(Scratchpad + frameIdx * 2, 0x4E) - S(Scratchpad, 0x0F2);
    S(actor, 0x3CC) = (signed short)(dy / frames);
    func_8005C454(actor + 0x3CC, 0x10, -8);

    S(actor, 0x3CE) = (signed short)func_8001E5A4(lungeHeading, stepSpeed);

    H(Scratchpad, 0x0F0) += (unsigned short)(S(actor, 0x3CA) * frames);
    H(Scratchpad, 0x0F2) += (unsigned short)(S(actor, 0x3CC) * frames);
    H(Scratchpad, 0x0F4) += (unsigned short)(S(actor, 0x3CE) * frames);

    if ((unsigned int)(S(Scratchpad, 0x0F2) - S(Scratchpad + frameIdx * 2, 0x4E) + 0xFF) >= 0x1BFu) {
        return 0;
    }

    if ((func_8003CEE4(actor, frameIdx, 0, 1, 0x14400) & 0xFFu) == 0u) {
        return 0;
    }

    func_8005BA30(actor, 0x31, 0);
    newHeading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
    B(actor, 0x3AA) = (unsigned char)newHeading;
    rot = 0x1C0 - (int)(newHeading & 0xFFu);
    H(actor, 0x26) = (unsigned short)((rot % 256) * 16);

    B(actor, 0x3D1) = 0;
    H(actor, 0x2) += H(actor, 0x3CA);
    H(actor, 0x4) += H(actor, 0x3CC);
    H(actor, 0x6) += H(actor, 0x3CE);
    B(actor, 0x3B2) = (unsigned char)aimHeading;

    row = (unsigned int)B(actor, 0x392) * 12u + (unsigned int)B(actor, 0x398) * 264u;
    B(actor, 0x3AC) = (unsigned char)((W(D_8010C330, row) >> 4) & 1u);
    return 1;
}
