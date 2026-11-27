/*
 * func_8003A174
 *
 * Purpose:
 *   Per-frame state machine for an aerial/diving action (such as a jumping
 *   header or volley clearance) driven by actor+0x397 (sub-steps 0..3):
 *     - Step 0: Turns heading toward +0x3AB and waits for anim frame +0x390 != 0.
 *     - Step 1: Applies velocity (+0x3CA/+0x3CC/+0x3CE), checks contact at
 *       +0x390 >= 8 via func_8003D274; on hit, triggers opponent reaction
 *       (func_8003D99C), plays SFX 0x507, kicks the ball via func_8004B0F0,
 *       sets ball+0x396 = 4 and spin, and calls func_8002B5A4 / func_800570F0.
 *     - Step 2: Applies position delta and damps y (*8/10) and xz velocity (*9/10)
 *       until +0x390 == 0xE, when y is snapped to 0.
 *     - Step 3: Waits for +0x390 == 0x2E, then clears +0x396 and +0x397 to 0.
 *
 * Arguments:
 *   actor (unsigned char *): pointer to 1000-byte player actor struct
 *
 * Return value:
 *   void
 *
 * Scratchpad / Actor fields read & written:
 *   Scratchpad 0x2F4 (ptr, R): ball actor pointer
 *   Actor +0x02/+0x04/+0x06 (s16, R/W): world x, y, z
 *   Actor +0x26  (s16, W): model yaw ((0x1C0 - heading) % 256) << 4
 *   Actor +0x390 (u8,  R): animation progress/frame index
 *   Actor +0x396 (u8,  W): actor state (cleared to 0 at end)
 *   Actor +0x397 (u8, R/W): sub-step (0..3)
 *   Actor +0x3AA (u8, R/W): body heading
 *   Actor +0x3AB (u8,  R): target heading
 *   Actor +0x3CA/+0x3CC/+0x3CE (s16, R/W): jump/motion step deltas
 *   Actor +0x3D1 (u8,  W): set to 3 on contact step
 *   Ball +0x396  (u8,  W): set to 4 on ball contact
 *   Ball +0x3CA  (s16, W): random(3)
 *   Ball +0x3C8  (s16, W): 9 - ball+0x3CA
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern void         func_8005BD1C(unsigned char *actor);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);
extern unsigned int func_8003D274(unsigned char *actor, unsigned int arg1, unsigned int arg2);
extern void         func_8003D99C(unsigned char *actor, unsigned int heading, unsigned int arg2);
extern unsigned int func_8001E6B8(unsigned int n);
extern void         func_800AB620(unsigned int sfx_id);
extern void         func_8004B0F0(unsigned char *actor, unsigned int dir_flag, int power, int lift);
extern void         func_8002B5A4(unsigned char *actor, unsigned int arg1);
extern void         func_800570F0(unsigned char *actor);

void func_8003A174(unsigned char *actor) {
    unsigned char substate = B(actor, 0x397);

    switch (substate) {
    case 0: {
        unsigned int h;
        int yaw_raw;

        func_8005BD1C(actor);
        h = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0x0C);
        B(actor, 0x3AA) = (unsigned char)h;
        yaw_raw = 0x1C0 - (int)(h & 0xFFu);
        S(actor, 0x26) = (signed short)((yaw_raw % 256) << 4);
        if (B(actor, 0x390) != 0) {
            B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
        }
        break;
    }

    case 1: {
        unsigned int h;
        int yaw_raw;

        func_8005BD1C(actor);
        h = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0x0C);
        B(actor, 0x3AA) = (unsigned char)h;
        yaw_raw = 0x1C0 - (int)(h & 0xFFu);
        S(actor, 0x26) = (signed short)((yaw_raw % 256) << 4);

        H(actor, 0x02) = (unsigned short)(H(actor, 0x02) + H(actor, 0x3CA));
        H(actor, 0x04) = (unsigned short)(H(actor, 0x04) + H(actor, 0x3CC));
        H(actor, 0x06) = (unsigned short)(H(actor, 0x06) + H(actor, 0x3CE));

        if (B(actor, 0x390) >= 8) {
            if ((func_8003D274(actor, 1, 1) & 0xFFu) != 0) {
                unsigned int flag;
                int power;
                int lift;
                unsigned char *ball;
                unsigned int r;

                func_8003D99C(actor, (B(actor, 0x3AB) + 0x80u) & 0xFFu, 0);
                flag = func_8001E6B8(2);
                if (flag != 0) {
                    power = (int)func_8001E6B8(8) + 0x84;
                    lift = (int)func_8001E6B8(0x0C) + 2;
                } else {
                    power = (int)func_8001E6B8(8) + 0x8C;
                    lift = (int)func_8001E6B8(4) - 0x0D;
                }

                func_800AB620(0x507);
                func_8004B0F0(actor, flag & 0xFFu, power, lift);

                ball = (unsigned char *)W(SCRATCH, 0x2F4);
                B(ball, 0x396) = 4;
                r = func_8001E6B8(3);
                S(ball, 0x3CA) = (signed short)r;
                S(ball, 0x3C8) = (signed short)(9 - (int)r);

                func_8002B5A4(actor, 3);
                func_800570F0(actor);
            }
            B(actor, 0x3D1) = 3;
            B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
        }
        break;
    }

    case 2: {
        signed short vx = S(actor, 0x3CA);
        signed short vz = S(actor, 0x3CE);
        signed short y;

        func_8005BD1C(actor);
        y = S(actor, 0x04);
        H(actor, 0x02) = (unsigned short)(H(actor, 0x02) + (unsigned short)vx);
        H(actor, 0x06) = (unsigned short)(H(actor, 0x06) + (unsigned short)vz);
        S(actor, 0x04) = (signed short)((y * 8) / 10);
        S(actor, 0x3CA) = (signed short)((vx * 9) / 10);
        S(actor, 0x3CE) = (signed short)((vz * 9) / 10);

        if (B(actor, 0x390) == 0x0E) {
            S(actor, 0x04) = 0;
            B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
        }
        break;
    }

    case 3:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0x2E) {
            B(actor, 0x396) = 0;
            B(actor, 0x397) = 0;
        }
        break;

    default:
        break;
    }
}
