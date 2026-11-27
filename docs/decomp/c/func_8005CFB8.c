/*
 * func_8005CFB8 - AI off-ball support/covering movement relative to ball and goal
 *
 * Purpose:
 *   Computes a target pitch coordinate for an AI outfielder based on Scratchpad
 *   0x2CF[side] and designated-runner tables at Scratchpad 0x31E/0x31F:
 *   - If Scratchpad 0x2CF[side] == 0 and actor is designated at 0x31F[atk_dir]:
 *     moves 0xA00 ahead of the ball along the ball's movement heading.
 *   - If Scratchpad 0x2CF[side] != 0 and actor is designated at 0x31E[atk_dir]:
 *     tracks toward the goal (x = ±0x3320, z = 0) from either the opponent
 *     ball-carrier (distance 0x600) or the ball itself (distance 0xA00).
 *   Clamps target x to [-0x32E0, 0x32E0] and issues func_80068FC0(actor, x, z).
 *   Otherwise delegates to func_8005D28C(actor).
 *
 * Arguments:
 *   actor (u8*) - pointer to 1000-byte player actor struct
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Actor fields read:
 *   Scratch 0x2CF[2] (u8)  - team possession / defending mode flag per side
 *   Scratch 0x2F4    (u8*) - pointer to ball actor struct
 *   Scratch 0x31E[2] (u8)  - designated actor index (stride 2 bytes per attack dir)
 *   Scratch 0x31F[2] (u8)  - designated actor index (stride 2 bytes per attack dir)
 *   Actor +0x02      (s16) - x position
 *   Actor +0x06      (s16) - z position
 *   Actor +0x38D     (u8)  - actor ID
 *   Actor +0x398     (u8)  - side (0/1)
 *   Actor +0x399     (u8)  - attack direction flag (0 -> goal +0x3320, 1 -> goal -0x3320)
 *   Actor +0x3A4     (u8)  - move heading
 *   Actor +0x3AB     (u8)  - target heading
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define H(p, o) (*(unsigned short *)((p) + (o)))
#define S(p, o) (*(signed short *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];

extern int func_8001E694(unsigned int angle, unsigned int mag);
extern int func_8001E5A4(unsigned int angle, unsigned int mag);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int current, unsigned int step);
extern void func_80068FC0(unsigned char *actor, int x, int z);
extern void func_8005D28C(unsigned char *actor);

void func_8005CFB8(unsigned char *actor)
{
    unsigned char side = B(actor, 0x398);
    int target_x;
    int target_z;

    if (B(Scratchpad, 0x2CF + side) == 0) {
        if (B(actor, 0x38D) != B(Scratchpad, 0x31F + (B(actor, 0x399) << 1))) {
            func_8005D28C(actor);
            return;
        }
        {
            unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
            unsigned char ball_dir = B(ball, 0x3A4);
            int dx = func_8001E694(ball_dir, 0xA00);
            int dz;

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            target_x = (signed short)(H(ball, 0x02) + dx);
            dz = func_8001E5A4(B(ball, 0x3A4), 0xA00);
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            target_z = (signed short)(H(ball, 0x06) + dz);
        }
    } else {
        unsigned char atk_dir = B(actor, 0x399);
        unsigned int opp_idx;
        unsigned char *opp;
        unsigned char *ball;
        unsigned int opp_dir;
        unsigned int diff;
        int facing_goal;
        unsigned int angle;
        unsigned int dist;
        int dx, dz;

        if (B(actor, 0x38D) != B(Scratchpad, 0x31E + (atk_dir << 1))) {
            func_8005D28C(actor);
            return;
        }

        opp_idx = B(Scratchpad, 0x31F + ((atk_dir ^ 1) << 1)) % 24U;
        opp = D_800FF748 + opp_idx * 0x3E8;
        ball = (unsigned char *)W(Scratchpad, 0x2F4);

        opp_dir = B(opp, 0x399) << 7;
        diff = (opp_dir - B(ball, 0x3A4)) & 0xFF;
        if (diff < 0x81) {
            facing_goal = (diff < 0x41);
        } else {
            facing_goal = (((B(ball, 0x3A4) - opp_dir) & 0xFF) < 0x41);
        }

        if (facing_goal) {
            int goal_x = (B(opp, 0x399) != 0) ? -0x3320 : 0x3320;
            angle = func_8005B36C(S(opp, 0x02), S(opp, 0x06), goal_x, 0);
            dist = 0x600;
        } else {
            int goal_x = (B(opp, 0x399) != 0) ? -0x3320 : 0x3320;
            unsigned int opp2_idx;
            unsigned char *opp2;

            angle = func_8005B36C(S(ball, 0x02), S(ball, 0x06), goal_x, 0);
            opp2_idx = B(Scratchpad, 0x31E + ((B(actor, 0x399) ^ 1) << 1)) % 24U;
            opp2 = D_800FF748 + opp2_idx * 0x3E8;
            angle = func_8005B7CC(angle & 0xFF, B(opp2, 0x3AB), 0x10);
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            dist = 0xA00;
        }

        angle &= 0xFF;
        dx = func_8001E694(angle, dist);
        dz = func_8001E5A4(angle, dist);
        target_x = (signed short)(H(ball, 0x02) + dx);
        target_z = (signed short)(H(ball, 0x06) + dz);
    }

    if (target_x >= 0x32E1) {
        target_x = 0x32E0;
    }
    if (target_x < -0x32E0) {
        target_x = -0x32E0;
    }

    func_80068FC0(actor, (signed short)target_x, (signed short)target_z);
}
