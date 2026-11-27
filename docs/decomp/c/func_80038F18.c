/*
 * func_80038F18
 *
 * Purpose:
 *   Steers an off-ball player toward the scratchpad target (0xF8, 0xFC) with
 *   optional (dx, dz) offset and pitch-length clamping, either settling into
 *   idle/facing the ball within 0.256 m (64 raw), jogging/positioning via
 *   func_80048FC4 when close (< 1.024 m and < 41 frames away), or running via
 *   func_800479B8; then transitions state to 0x1B/1 if ball owner or 0/0 if
 *   play is not open.
 *
 * Arguments:
 *   actor: (unsigned char*) 1000-byte player actor struct
 *   dx:    (signed short) X offset added to Scratchpad+0xF8 when nonzero
 *   dz:    (signed short) Z offset added to Scratchpad+0xFC when dx != 0
 *   arg3:  (unsigned char) speed/mode parameter passed to func_800484F8
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   actor+0x02/+0x06 (s16): player X/Z position (R)
 *   actor+0x38D (u8): actor ID (R)
 *   actor+0x38E (u16): current animation ID (R)
 *   actor+0x396/+0x397 (u8): state / substate (W)
 *   actor+0x3A0 (u32): speed, decelerated by 18 per frame when < 64 raw (R/W)
 *   actor+0x3A4/+0x3AA (u8): move / body heading (R/W)
 *   actor+0x3B4 (u8): max speed (R)
 *   Scratchpad+0x36/+0x66 (s16): reference/ball X/Z for facing angle (R)
 *   Scratchpad+0xF8/+0xFC (s16): target X/Z, offset and X-clamped to [-0x3320, 0x3320] (R/W)
 *   Scratchpad+0x29A (u8): match phase (1 = open play) (R)
 *   Scratchpad+0x2A9 (u8): ball owner actor ID (R)
 *   Scratchpad+0x2FC (u8): override reset flag (R)
 *   Scratchpad+0x31C (s16): ball timer (frames since last touch) (R)
 *
 * Magic numbers:
 *   0x3320 (13088 raw = 52.352 m): pitch X half-length clamp.
 *   0x40 (64 raw = 0.256 m): arrival radius to stop and face ball.
 *   0x100 (256 raw = 1.024 m), 0x29 (41 frames = 1.37 s): fine-positioning threshold.
 *   0x18 (24 frames = 0.80 s): ball timer threshold for func_800479B8 sprint flag.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned int func_800484F8(signed short x, signed short z, unsigned char maxSpd, unsigned int arg3);
extern unsigned int func_8005B36C(signed short x1, signed short z1, signed short x2, signed short z2);
extern unsigned int func_800B1128(unsigned int sq);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, int step);
extern void func_80048FC4(unsigned char *actor, unsigned int moveAng, unsigned int faceAng, unsigned int dist);
extern void func_800479B8(unsigned char *actor, int sprint);

void func_80038F18(unsigned char *actor, int dx, int dz, unsigned int arg3) {
    signed int moveSpd;
    signed short tx;
    unsigned int ballAng;
    signed int rx;
    signed int rz;
    unsigned int dist;

    moveSpd = (signed int)(func_800484F8(S(actor, 2), S(actor, 6), B(actor, 0x3B4), arg3 & 0xFFu) & 0xFFu);

    if ((signed short)dx != 0) {
        H(Scratchpad, 0xF8) = (unsigned short)(H(Scratchpad, 0xF8) + dx);
        H(Scratchpad, 0xFC) = (unsigned short)(H(Scratchpad, 0xFC) + dz);
    }

    tx = S(Scratchpad, 0xF8);
    if (tx >= 0x3320) {
        H(Scratchpad, 0xF8) = 0x3320;
    } else if (tx < -0x331F) {
        H(Scratchpad, 0xF8) = (unsigned short)(-0x3320);
    }

    ballAng = func_8005B36C(S(actor, 2), S(actor, 6), S(Scratchpad, 0x36), S(Scratchpad, 0x66));
    rx = S(actor, 2) - S(Scratchpad, 0xF8);
    rz = S(actor, 6) - S(Scratchpad, 0xFC);
    dist = func_800B1128((unsigned int)(rx * rx + rz * rz));

    if (dist < 0x40u) {
        unsigned int newAng;
        unsigned int spd;

        if (H(actor, 0x38E) != 1u) {
            func_8005BA30(actor, 1, 0);
        }
        newAng = func_8005B7CC(ballAng & 0xFFu, B(actor, 0x3AA), 0x10);
        B(actor, 0x3AA) = (unsigned char)newAng;
        B(actor, 0x3A4) = (unsigned char)newAng;
        spd = W(actor, 0x3A0);
        W(actor, 0x3A0) = (spd < 0x13u) ? 0u : (spd - 0x12u);
    } else {
        unsigned int targetAng = func_8005B36C(S(actor, 2), S(actor, 6), S(Scratchpad, 0xF8), S(Scratchpad, 0xFC));
        unsigned int estFrames;

        if (moveSpd < 9) {
            moveSpd = 1;
        } else {
            moveSpd -= 8;
        }
        estFrames = dist / (unsigned int)moveSpd;
        if (H(actor, 0x38E) == 4u) {
            estFrames += 8u;
        }

        if (dist < 0x100u && estFrames < 0x29u) {
            func_80048FC4(actor, targetAng & 0xFFu, ballAng & 0xFFu, dist & 0xFFFFu);
        } else {
            func_800479B8(actor, (S(Scratchpad, 0x31C) < 0x18) ? 0 : 1);
        }
    }

    if (B(actor, 0x38D) == B(Scratchpad, 0x2A9)) {
        B(actor, 0x396) = 0x1B;
        B(actor, 0x397) = 1;
    } else if (B(Scratchpad, 0x29A) != 1u || B(Scratchpad, 0x2FC) != 0u) {
        B(actor, 0x396) = 0;
        B(actor, 0x397) = 0;
    }
}
