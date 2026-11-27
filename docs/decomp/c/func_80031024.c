/*
 * func_80031024
 *
 * Purpose:
 *   Updates a ball-carrying player's action state from queued command +0x3CC /
 *   +0x3CA / +0x38E (handling trap/dribble/kick/turn transitions via
 *   func_8002C28C, func_8004A07C + func_8005B7CC, or func_8005BA78), and then
 *   repositions the ball 0x40 (64) units in front of the player if the ball
 *   is behind them or >= 0x2400 (96^2) squared units away.
 *
 * Arguments:
 *   actor (u8*): player actor struct pointer
 *
 * Return value:
 *   None (void)
 *
 * Actor fields read/written:
 *   +0x002 (s16) X position (R)
 *   +0x006 (s16) Z position (R)
 *   +0x38E (u16) turn/move flag (R)
 *   +0x390 (u8)  animation param for func_8005BA78 (R)
 *   +0x396 (u8)  state (W)
 *   +0x397 (u8)  substate (W)
 *   +0x3A4 (u8)  movement direction (W)
 *   +0x3AA (u8)  facing angle (R/W)
 *   +0x3B0 (u8)  flag (W: 0)
 *   +0x3B1 (u8)  flag (W: 0)
 *   +0x3C2 (s16) timer (W: 0)
 *   +0x3C4 (s16) timer (W: 4)
 *   +0x3CA (s16) queued action flag (R)
 *   +0x3CC (s16) queued state (R)
 *   +0x3D1 (u8)  flag (W: 0)
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x2F4 (u32) ball pointer (R/W +0x02 X, +0x06 Z)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern void func_80054A1C(unsigned char *actor);
extern void func_8002C28C(unsigned char *actor);
extern unsigned int func_8004A07C(unsigned char *actor);
extern unsigned char func_8005B7CC(unsigned char target, unsigned char cur, int step);
extern void func_8005BA78(unsigned char *actor, unsigned int anim, unsigned char param2);
extern unsigned int func_8005B36C(signed short x1, signed short z1, signed short x2, signed short z2);
extern signed int func_8001E694(unsigned char ang, int radius);
extern signed int func_8001E5A4(unsigned char ang, int radius);

void func_80031024(unsigned char *actor)
{
    signed short next_state;
    unsigned char *ball;
    unsigned int ang;
    unsigned char diff;
    int push_ball = 0;

    func_80054A1C(actor);

    next_state = S(actor, 0x3CC);
    if (next_state == 4) {
        B(actor, 0x396) = 4;
        S(actor, 0x3C2) = 0;
        B(actor, 0x3B0) = 0;
        B(actor, 0x3B1) = 0;
        B(actor, 0x3D1) = 0;
        B(actor, 0x397) = 1;
    } else if (next_state != 0xFF) {
        B(actor, 0x396) = (unsigned char)next_state;
        B(actor, 0x397) = 0;
    } else if (S(actor, 0x3CA) == 1) {
        S(actor, 0x3C4) = 4;
        B(actor, 0x3B0) = 0;
        B(actor, 0x396) = 5;
        B(actor, 0x397) = 0;
        B(actor, 0x3A4) = B(actor, 0x3AA);
        func_8002C28C(actor);
    } else if (H(actor, 0x38E) == 1) {
        unsigned char target_ang = (unsigned char)(func_8004A07C(actor) & 0xFFU);
        B(actor, 0x3AA) = func_8005B7CC(target_ang, B(actor, 0x3AA), 0x10);
        B(actor, 0x396) = 0;
        B(actor, 0x397) = 0;
    } else {
        func_8005BA78(actor, 0x34, B(actor, 0x390));
        B(actor, 0x396) = 1;
        B(actor, 0x397) = 0;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    ang = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));
    if (((ang - B(actor, 0x3AA)) & 0xFFU) < 0x81U) {
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        ang = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));
        diff = (unsigned char)((ang - B(actor, 0x3AA)) & 0xFFU);
    } else {
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        ang = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));
        diff = (unsigned char)((B(actor, 0x3AA) - ang) & 0xFFU);
    }

    if (diff >= 0x40) {
        push_ball = 1;
    } else {
        signed int dx, dz;
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        dx = S(actor, 2) - S(ball, 2);
        dz = S(actor, 6) - S(ball, 6);
        if (dx * dx + dz * dz >= 0x2400) {
            push_ball = 1;
        }
    }

    if (push_ball) {
        signed int ox = func_8001E694(B(actor, 0x3AA), 0x40);
        signed int oz;
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        H(ball, 2) = (unsigned short)(H(ball, 2) + ox);
        oz = func_8001E5A4(B(actor, 0x3AA), 0x40);
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        H(ball, 6) = (unsigned short)(H(ball, 6) + oz);
    }
}
