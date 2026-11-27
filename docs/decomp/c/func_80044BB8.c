/*
 * func_80044BB8
 *
 * Purpose:
 *   Goalkeeper / player goal-approach or set-piece positioning AI step.
 *   Marks actor flags (+0x3A7, +0x3D1), handles state 3 early-frame cutoff via
 *   func_80049F90, tracks distance to goal (+-0x3300, actor+0x3C6) and ball
 *   angle (Scratchpad+0x36, +0x66), transitions actor+0x397 (0 -> 1) when within
 *   0x80 units, and updates actor+0x3D4/+0x3D6 during state 4.
 *
 * Arguments:
 *   actor (unsigned char *): pointer to 1000-byte (0x3E8) actor struct
 *
 * Return value:
 *   None (void)
 *
 * Actor fields read:
 *   +0x02 (s16) X position, +0x06 (s16) Z position,
 *   +0x38E (u16) action/anim state, +0x397 (u8) sub-state (0, 1),
 *   +0x398 (u8) pad/player index, +0x399 (u8) team side (0 -> goal -0x3300, nonzero -> +0x3300),
 *   +0x39A (u8) flag, +0x3A0 (u32) timer/counter, +0x3AA (u8) facing angle,
 *   +0x3C6 (s16) target Z position
 *
 * Actor fields written:
 *   +0x3A7 (u8) = 1, +0x3D1 (u8) = 1,
 *   +0x3AA (u8) facing angle, +0x397 (u8) = 1, +0x3A0 (u32),
 *   +0x3D4 (u16), +0x3D6 (u16)
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x12[idx] (u16), +0x36 (s16) ball X, +0x66 (s16) ball Z,
 *   +0x2B0 (u32) match sub-state (3, 4), +0x31C (s16) phase timer, +0x328 (u8) flag
 *
 * Scratchpad fields written:
 *   +0xF8 (s16) target goal X (+-0x3300), +0xFC (s16) target Z
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern void func_8005BD1C(unsigned char *actor);
extern void func_80049F90(unsigned char *actor);
extern unsigned int func_8005B36C(signed short x0, signed short z0, signed short x1, signed short z1);
extern unsigned int func_800B1128(unsigned int val);
extern void func_8005BA30(unsigned char *actor, unsigned int a1, unsigned int a2);
extern unsigned char func_8005B7CC(unsigned int targetAngle, unsigned int curAngle, unsigned int step);
extern void func_80048AD8(unsigned char *actor, unsigned int goalAngle, unsigned int ballAngle, unsigned int dist);
extern void func_800479B8(unsigned char *actor, unsigned int a1);
extern void func_80045504(unsigned char *actor, unsigned int a1);
extern void func_8005B908(unsigned char *actor);

void func_80044BB8(unsigned char *actor)
{
    signed short goalX;
    signed short targetZ;
    unsigned int ballAngle;
    int dx;
    int dz;
    unsigned int dist;
    unsigned char subState;

    B(actor, 0x3A7) = 1;
    B(actor, 0x3D1) = 1;
    func_8005BD1C(actor);

    if (W(Scratchpad, 0x2B0) == 3 && S(Scratchpad, 0x31C) < 8) {
        func_80049F90(actor);
        func_8005B908(actor);
        return;
    }

    goalX = 0x3300;
    ballAngle = func_8005B36C(S(actor, 2), S(actor, 6), S(Scratchpad, 0x36), S(Scratchpad, 0x66));
    if (B(actor, 0x399) == 0) {
        goalX = -0x3300;
    }

    targetZ = S(actor, 0x3C6);
    dx = (int)S(actor, 2) - (int)goalX;
    dz = (int)S(actor, 6) - (int)targetZ;
    dist = func_800B1128((unsigned int)(dx * dx + dz * dz));

    subState = B(actor, 0x397);
    if (subState == 0) {
        if (dist < 0x80U) {
            if (H(actor, 0x38E) != 1) {
                func_8005BA30(actor, 1, 0);
            }
            B(actor, 0x3AA) = func_8005B7CC(ballAngle & 0xFF, B(actor, 0x3AA), 0x10);
            B(actor, 0x397) = 1;
        } else if (dist < 0x200U) {
            unsigned int goalAngle = func_8005B36C(S(actor, 2), S(actor, 6), goalX, targetZ);
            func_80048AD8(actor, goalAngle & 0xFF, ballAngle & 0xFF, dist & 0xFFFF);
        } else {
            S(Scratchpad, 0xF8) = goalX;
            S(Scratchpad, 0xFC) = targetZ;
            func_800479B8(actor, 1);
        }
    } else if (subState == 1) {
        unsigned int timer;
        B(actor, 0x3AA) = func_8005B7CC(ballAngle & 0xFF, B(actor, 0x3AA), 0x10);
        timer = W(actor, 0x3A0);
        if (timer < 0x13U) {
            W(actor, 0x3A0) = 0;
        } else {
            W(actor, 0x3A0) = timer - 0x12U;
        }
    }

    if (B(actor, 0x39A) == 0 || B(Scratchpad, 0x328) != 0) {
        if (W(Scratchpad, 0x2B0) == 4 && S(Scratchpad, 0x31C) >= 0x10) {
            unsigned short val = H(Scratchpad, 0x12 + ((unsigned int)B(actor, 0x398) << 1));
            H(actor, 0x3D4) = val;
            H(actor, 0x3D6) = val;
        }
    }

    func_80045504(actor, 0x13);
    func_8005B908(actor);
}
