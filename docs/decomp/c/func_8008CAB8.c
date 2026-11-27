/*
 * func_8008CAB8
 *
 * Purpose:
 *   Goalkeeper lateral parry/deflection state update. Handles reaction delay
 *   based on GK stat (+0x3B9 / 10) vs ball flight timer (SCRATCH+0x31C),
 *   plays GK animation 0x65 then transitions to 0x62 (state 0x8D) when
 *   +0x390 == 0, steps laterally (+/-90 deg from body heading) at speed 12,
 *   computes a height-dependent deflection angle and calls func_8009462C to
 *   deflect the ball (followed by func_80057A14 on contact).
 *
 * Arguments:
 *   actor (unsigned char *): pointer to 1000-byte goalkeeper actor struct
 *
 * Return value:
 *   void
 *
 * Scratchpad / Actor / Global fields read & written:
 *   SCRATCH 0x036/0x04E/0x066 (s16, R): ball predicted frame-0 xyz
 *   SCRATCH 0x2AC (u8,  R): shot/deflection actor ID check
 *   SCRATCH 0x2F4 (ptr, R): ball actor pointer (+0x0C vy, +0x3A4 heading)
 *   SCRATCH 0x31C (s16, R): ball flight timer (frames since last touch)
 *   D_8010C330[side*264 + slot*12] (u32, R): bits 11..17 = height - 100 cm
 *   Actor +0x02/+0x06 (s16, R/W): x, z position
 *   Actor +0x26  (s16, W): model yaw
 *   Actor +0x38D (u8,  R): actor ID
 *   Actor +0x390 (u8,  R): animation frame/phase
 *   Actor +0x392 (u8,  R): roster slot
 *   Actor +0x396 (u8,  W): actor state (set to 0x8D)
 *   Actor +0x397 (u8, R/W): sub-step
 *   Actor +0x398 (u8,  R): side (0/1)
 *   Actor +0x3A0 (u32, R/W): GK timer/cooldown (decremented by 18)
 *   Actor +0x3AA (u8, R/W): body heading
 *   Actor +0x3AB (u8,  R): target heading
 *   Actor +0x3AC (u8,  R): side/direction flag (non-zero = +0x40, zero = -0x40)
 *   Actor +0x3B9 (u8,  R): GK level/defense stat (divided by 10 for react delay)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned char D_8010C330[];

extern void         func_8006EC5C(unsigned char *actor);
extern void         func_8005BA30(unsigned char *actor, unsigned int anim_id, unsigned int arg2);
extern void         func_8005BD1C(unsigned char *actor);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);
extern int          func_8001E694(unsigned int angle, unsigned int speed);
extern int          func_8001E5A4(unsigned int angle, unsigned int speed);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_8001E6B8(unsigned int n);
extern unsigned int func_8009462C(unsigned char *actor, int mode, int heading, int pitch, int lift);
extern void         func_80057A14(unsigned char *actor);
extern void         func_8005B864(unsigned char *actor, unsigned short arg1);
extern void         func_800954E8(unsigned char *actor);

void func_8008CAB8(unsigned char *actor) {
    unsigned int h;
    int yaw_raw;
    unsigned int lat_angle;
    int dx, dz;
    unsigned int ball_ang;
    int sign;
    unsigned int stat_off;
    int height_cm;
    int height_thresh;
    int defl_offset;
    int defl_heading;
    unsigned char *ball;
    int ball_vy_scaled;
    int r0, r1;

    if (B(actor, 0x397) == 0) {
        int react_delay = 14 - (int)(B(actor, 0x3B9) / 10u);
        if (S(SCRATCH, 0x31C) < react_delay && B(actor, 0x38D) != B(SCRATCH, 0x2AC)) {
            unsigned int cd;
            func_8006EC5C(actor);
            cd = W(actor, 0x3A0);
            W(actor, 0x3A0) = (cd < 0x13u) ? 0u : (cd - 0x12u);
        } else {
            func_8005BA30(actor, 0x65, 0);
            W(actor, 0x3A0) = 0;
            B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
        }
    } else {
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0) {
            func_8005BA30(actor, 0x62, 0x17);
            B(actor, 0x396) = 0x8D;
            B(actor, 0x397) = 1;
        }
    }

    h = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0x0C);
    B(actor, 0x3AA) = (unsigned char)h;
    yaw_raw = 0x1C0 - (int)(h & 0xFFu);
    S(actor, 0x26) = (signed short)((yaw_raw % 256) << 4);

    if (B(actor, 0x3AC) != 0) {
        lat_angle = (unsigned int)B(actor, 0x3AA) + 0x40u;
    } else {
        lat_angle = (unsigned int)B(actor, 0x3AA) - 0x40u;
    }
    lat_angle &= 0xFFu;

    dx = func_8001E694(lat_angle, 0x0C);
    H(actor, 0x02) = (unsigned short)(H(actor, 0x02) + (unsigned short)dx);
    dz = func_8001E5A4(lat_angle, 0x0C);
    H(actor, 0x06) = (unsigned short)(H(actor, 0x06) + (unsigned short)dz);

    ball_ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(SCRATCH, 0x036), S(SCRATCH, 0x066));
    if (((ball_ang - B(actor, 0x3AA)) & 0xFFu) < 0x81u) {
        sign = 1;
    } else {
        sign = -1;
    }

    stat_off = (unsigned int)B(actor, 0x392) * 12u + (unsigned int)B(actor, 0x398) * 264u;
    height_cm = (int)(((W(D_8010C330, stat_off) >> 11) & 0x7Fu) + 100u);
    height_thresh = -(height_cm * 160) / 180;

    if (S(SCRATCH, 0x04E) < height_thresh) {
        defl_offset = (int)func_8001E6B8(0x14) + 0x34;
    } else {
        defl_offset = (int)func_8001E6B8(0x40) + 0x30;
    }

    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    defl_heading = (int)B(ball, 0x3A4) + sign * defl_offset;

    /* Note: computes abs(ball_vy / 736) into $a0 in asm, then overwrites $a0 with 4 */
    ball_vy_scaled = SW(ball, 0x0C) / 736;
    if (ball_vy_scaled < 0) {
        ball_vy_scaled = -ball_vy_scaled;
    }
    (void)ball_vy_scaled;

    r0 = (int)func_8001E6B8(4);
    /* UNCLEAR: second call to func_8001E6B8 in asm does not reload $a0; relies on $a0 surviving first call */
    r1 = (int)func_8001E6B8(4);

    if ((func_8009462C(actor, 0x0C, defl_heading & 0xFF, 0, r1 - r0) & 0xFFu) != 0) {
        func_80057A14(actor);
    }

    func_8005B864(actor, H(actor, 0x3A0));
    func_800954E8(actor);
}
