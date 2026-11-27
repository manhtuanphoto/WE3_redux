/*
 * func_8008EFA4
 *
 * Purpose:
 *   Goalkeeper dive/catch/parry movement and ball-contact state update.
 *   - Substate 0: Starts GK animation 0x60, turns rapidly toward +0x3AB
 *     (step 0x40), applies velocity (+0x08/+0x10), increments +0x397, returns.
 *   - Substate 1+: Holds for 16 frames when +0x390 == 0x14 (using +0x3B0),
 *     advances animation via func_8005BD1C, moves along velocity for +0x390 in
 *     1..6, and on +0x390 == 6 tests GK touch via func_80094EEC(actor, 0x100).
 *     If touched, either punches/spills via func_8004B0F0 + func_80095090
 *     (when ball speed >= 0x7801, ball timer < 45, and rand(3) == 0) or catches
 *     via func_80094FF4 + func_8005778C.
 *   - Transitions to state 0x85 when +0x390 == +0x3CC (playing anim 0x5D at
 *     frame 0x23), or to state 0x82 if ball is owned by GK's side.
 *
 * Arguments:
 *   actor (unsigned char *): pointer to 1000-byte goalkeeper actor struct
 *
 * Return value:
 *   void
 *
 * Scratchpad / Actor fields read & written:
 *   SCRATCH 0x2A9 (u8,  R): current ball owner
 *   SCRATCH 0x2B0 (u32, W): ball event code (cleared to 0 on punch/spill)
 *   SCRATCH 0x2CF[2] (u8, R/W): team possession flag
 *   SCRATCH 0x2F4 (ptr, R): ball actor pointer (+0x3A0 speed)
 *   SCRATCH 0x31C (s16, R): ball flight timer (frames since last touch)
 *   Actor +0x02/+0x06 (s16, R/W): x, z position
 *   Actor +0x08/+0x10 (s32, R): x, z velocity added per step
 *   Actor +0x26  (s16, W): model yaw
 *   Actor +0x390 (u8,  R): animation phase/frame
 *   Actor +0x396 (u8,  W): actor state (0x85 or 0x82)
 *   Actor +0x397 (u8, R/W): sub-step
 *   Actor +0x398 (u8,  R): side (0/1)
 *   Actor +0x3AA/+0x3AB (u8, R/W): body/target heading
 *   Actor +0x3B0 (u8, R/W): pause frame counter while +0x390 == 0x14
 *   Actor +0x3CC (s16, R/W): end-of-dive frame threshold (set to 0x23 = 35)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern void         func_8005BA30(unsigned char *actor, unsigned int anim_id, unsigned int arg2);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);
extern void         func_8005BD1C(unsigned char *actor);
extern int          func_80094EEC(unsigned char *actor, int radius);
extern unsigned int func_8001E6B8(unsigned int n);
extern void         func_8004B0F0(unsigned char *actor, unsigned int heading, int power, int lift);
extern void         func_80095090(unsigned char *actor);
extern void         func_80094FF4(unsigned char *actor);
extern void         func_8005778C(unsigned char *actor);
extern void         func_800954E8(unsigned char *actor);

void func_8008EFA4(unsigned char *actor) {
    unsigned char substate = B(actor, 0x397);
    unsigned char phase;

    if (substate == 0) {
        unsigned int h;
        int yaw_raw;

        func_8005BA30(actor, 0x60, 0);
        h = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0x40);
        B(actor, 0x3AA) = (unsigned char)h;
        yaw_raw = 0x1C0 - (int)(h & 0xFFu);
        S(actor, 0x26) = (signed short)((yaw_raw % 256) << 4);
        H(actor, 0x02) = (unsigned short)(H(actor, 0x02) + W(actor, 0x08));
        H(actor, 0x06) = (unsigned short)(H(actor, 0x06) + W(actor, 0x10));
        B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
        return;
    }

    if (substate == 1) {
        if (B(actor, 0x390) == 0x14) {
            unsigned char cnt = (unsigned char)(B(actor, 0x3B0) + 1);
            B(actor, 0x3B0) = cnt;
            if (cnt < 0x10u) {
                return;
            }
        }

        func_8005BD1C(actor);
        phase = B(actor, 0x390);
        if (phase != 0) {
            if (phase < 6) {
                unsigned int h = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0x0C);
                int yaw_raw = 0x1C0 - (int)(h & 0xFFu);
                B(actor, 0x3AA) = (unsigned char)h;
                S(actor, 0x26) = (signed short)((yaw_raw % 256) << 4);
                H(actor, 0x02) = (unsigned short)(H(actor, 0x02) + W(actor, 0x08));
                H(actor, 0x06) = (unsigned short)(H(actor, 0x06) + W(actor, 0x10));
            } else if (phase == 6) {
                unsigned int h = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0x0C);
                int yaw_raw = 0x1C0 - (int)(h & 0xFFu);
                B(actor, 0x3AA) = (unsigned char)h;
                S(actor, 0x26) = (signed short)((yaw_raw % 256) << 4);
                H(actor, 0x02) = (unsigned short)(H(actor, 0x02) + W(actor, 0x08));
                H(actor, 0x06) = (unsigned short)(H(actor, 0x06) + W(actor, 0x10));

                if (func_80094EEC(actor, 0x100) != 0) {
                    unsigned char *ball = (unsigned char *)W(SCRATCH, 0x2F4);
                    int do_catch = 1;

                    if (W(ball, 0x3A0) >= 0x7801u && S(SCRATCH, 0x31C) < 0x2D) {
                        if (func_8001E6B8(3) == 0) {
                            unsigned int r = func_8001E6B8(6);
                            func_8004B0F0(actor, B(actor, 0x3AA), (int)r + 0x1C, 1);
                            func_80095090(actor);
                            B(SCRATCH, 0x2CFu + B(actor, 0x398)) = 0;
                            W(SCRATCH, 0x2B0) = 0;
                            do_catch = 0;
                        } else {
                            S(actor, 0x3CC) = 0x23;
                        }
                    } else if (S(SCRATCH, 0x31C) < 0x1D) {
                        S(actor, 0x3CC) = 0x23;
                    }

                    if (do_catch) {
                        func_80094FF4(actor);
                        func_8005778C(actor);
                    }
                }
            }
        }
    }

    phase = B(actor, 0x390);
    if ((int)phase == S(actor, 0x3CC)) {
        if (phase == 0x23) {
            func_8005BA30(actor, 0x5D, 4);
        }
        B(actor, 0x396) = 0x85;
        B(actor, 0x397) = 0;
    }

    if (B(SCRATCH, 0x2A9) != 0 && B(SCRATCH, 0x2CFu + B(actor, 0x398)) == 1) {
        B(actor, 0x396) = 0x82;
        B(actor, 0x397) = 0;
    }

    func_800954E8(actor);
}
