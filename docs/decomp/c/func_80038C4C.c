/*
 * func_80038C4C - Execute lunge / interception motion toward predicted ball point
 *
 * Purpose:
 *   Computes a blended facing angle between `target_angle` and opposite the ball's
 *   movement heading (Ball +0x3A4 + 0x80), initializes animation 0x1C, calculates
 *   the 5-frame velocity vector from Scratchpad (0x0F0, 0x0F4) toward (0x03E, 0x06E)
 *   clamped to speed 34 (0x22), advances Scratchpad (0x0F0, 0x0F4) by 5 * velocity,
 *   and calls func_8003CEE4. If func_8003CEE4 succeeds, commits the animation,
 *   updates model rotation, sets Actor +0x3D1 = 3, updates position/headings/foot
 *   flag, and returns 1; otherwise returns 0.
 *
 * Arguments:
 *   actor        (u8*)         - pointer to 1000-byte player actor struct
 *   target_angle (unsigned int) - desired facing angle (0..255)
 *
 * Return value:
 *   unsigned int - 1 if func_8003CEE4 succeeded and motion was committed, 0 otherwise.
 *
 * Scratchpad / Actor fields read & written:
 *   Scratch 0x03E (s16) - predicted/target x position
 *   Scratch 0x06E (s16) - predicted/target z position
 *   Scratch 0x0F0 (s16) - actor/intercept start x position (advanced by 5 * vx)
 *   Scratch 0x0F4 (s16) - actor/intercept start z position (advanced by 5 * vz)
 *   Scratch 0x2F4 (u8*) - pointer to ball actor struct (reads Ball +0x3A4)
 *   Actor +0x02   (s16) - x position (incremented by vx)
 *   Actor +0x06   (s16) - z position (incremented by vz)
 *   Actor +0x26   (s16) - model rotation ((0x1C0 - heading) % 256) << 4
 *   Actor +0x3A4  (u8)  - move heading
 *   Actor +0x3AA  (u8)  - body heading
 *   Actor +0x3AB  (u8)  - target heading
 *   Actor +0x3AC  (u8)  - mirror/foot flag (0 or 1 from ball-relative angle)
 *   Actor +0x3CA  (s16) - x velocity step
 *   Actor +0x3CE  (s16) - z velocity step
 *   Actor +0x3D1  (u8)  - set to 3 on success
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define H(p, o) (*(unsigned short *)((p) + (o)))
#define S(p, o) (*(signed short *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned int func_8005B7CC(unsigned int target, unsigned int current, unsigned int step);
extern void func_8005C2AC(unsigned char *actor, int anim_id, int frame, unsigned int heading, unsigned int foot);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_800B1128(unsigned int val);
extern int func_8001E694(unsigned int angle, unsigned int mag);
extern int func_8001E5A4(unsigned int angle, unsigned int mag);
extern unsigned int func_8003CEE4(unsigned char *actor, int arg1, int arg2, int arg3, int arg4);
extern void func_8005BA30(unsigned char *actor, int anim_id, int frame);

unsigned int func_80038C4C(unsigned char *actor, unsigned int target_angle)
{
    unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
    unsigned int opp_ball_dir = (B(ball, 0x3A4) + 0x80) & 0xFF;
    unsigned int diff = (B(ball, 0x3A4) - (target_angle + 0x80)) & 0xFF;
    unsigned int step;
    unsigned int blended_angle;
    int x1, z1, x2, z2;
    int dx, dz;
    unsigned int move_angle;
    unsigned int dist;
    unsigned int speed;
    int vx, vz;

    if (diff >= 0x81) {
        diff = ((target_angle + 0x80) - B(ball, 0x3A4)) & 0xFF;
    }
    step = (diff / 3) & 0xFF;

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    opp_ball_dir = (B(ball, 0x3A4) + 0x80) & 0xFF;
    blended_angle = func_8005B7CC(target_angle & 0xFF, opp_ball_dir, step);

    func_8005C2AC(actor, 0x1C, 0, blended_angle & 0xFF, B(actor, 0x3AC));

    x1 = S(Scratchpad, 0x0F0);
    z1 = S(Scratchpad, 0x0F4);
    x2 = S(Scratchpad, 0x03E);
    z2 = S(Scratchpad, 0x06E);
    move_angle = func_8005B36C(x1, z1, x2, z2);

    dx = S(Scratchpad, 0x0F0) - S(Scratchpad, 0x03E);
    dz = S(Scratchpad, 0x0F4) - S(Scratchpad, 0x06E);
    dist = func_800B1128((unsigned int)(dx * dx + dz * dz));

    speed = dist / 5U;
    if (speed >= 0x23U) {
        speed = 0x22U;
    }

    vx = func_8001E694(move_angle & 0xFF, speed);
    S(actor, 0x3CA) = (signed short)vx;
    vz = func_8001E5A4(move_angle & 0xFF, speed);
    S(actor, 0x3CE) = (signed short)vz;

    H(Scratchpad, 0x0F0) = (unsigned short)(H(Scratchpad, 0x0F0) + S(actor, 0x3CA) * 5);
    H(Scratchpad, 0x0F4) = (unsigned short)(H(Scratchpad, 0x0F4) + S(actor, 0x3CE) * 5);

    if ((func_8003CEE4(actor, 4, 1, 0, 0x14400) & 0xFF) == 0) {
        return 0;
    }

    func_8005BA30(actor, 0x1C, 0);
    {
        unsigned int body = func_8005B7CC(blended_angle & 0xFF, B(actor, 0x3AA), 0x10) & 0xFF;
        int rot;
        unsigned int rel_diff;

        B(actor, 0x3AA) = (unsigned char)body;
        rot = (0x1C0 - (int)body) % 256;
        S(actor, 0x26) = (signed short)(rot << 4);
        B(actor, 0x3D1) = 3;
        B(actor, 0x3AB) = (unsigned char)blended_angle;
        B(actor, 0x3A4) = (unsigned char)move_angle;

        H(actor, 0x02) = (unsigned short)(H(actor, 0x02) + H(actor, 0x3CA));
        H(actor, 0x06) = (unsigned short)(H(actor, 0x06) + H(actor, 0x3CE));

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        rel_diff = (blended_angle - B(ball, 0x3A4) - 0x80) & 0xFF;
        B(actor, 0x3AC) = (rel_diff < 0x81) ? 0 : 1;
    }

    return 1;
}
