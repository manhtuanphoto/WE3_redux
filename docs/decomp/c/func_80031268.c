/*
 * func_80031268
 *
 * Purpose:
 *   Executes a player's block/deflection/clearing action on the ball, computing
 *   kick power from the ball's current speed and player stat, kicking the ball
 *   via func_8004B0F0, or transitioning the actor into a follow-up state.
 *
 * Arguments:
 *   actor (u8*) - pointer to the 1000-byte player actor struct
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x2A8 (u8)  last-touch actor ID (R)
 *     +0x2A9 (u8)  ball owner ID (W)
 *     +0x2F4 (u8*) pointer to ball actor (R)
 *     +0x31C (s16) ball timer / side touch flag (W)
 *   Actor:
 *     +0x002 (s16) x position (R)
 *     +0x006 (s16) z position (R)
 *     +0x38D (u8)  actor ID (R)
 *     +0x38E (u16) current anim ID (R)
 *     +0x390 (u8)  anim frame (R)
 *     +0x392 (u8)  team slot 0..10 (R)
 *     +0x396 (u8)  state (W)
 *     +0x397 (u8)  substate (W)
 *     +0x398 (u8)  side 0/1 (R)
 *     +0x399 (u8)  attack direction flag (R)
 *     +0x3A0 (u32) speed (R)
 *     +0x3AA (u8)  body heading (R/W)
 *     +0x3AB (u8)  target heading (R)
 *     +0x3AC (u8)  mirror/foot flag (W)
 *     +0x3B0 (u8)  cleared to 0 on state 4 transition (W)
 *     +0x3B1 (u8)  cleared to 0 on state 4 transition (W)
 *     +0x3B4 (u8)  max speed (R)
 *     +0x3C2 (u16) cleared to 0 on state 4 transition (W)
 *     +0x3CC (s16) pending/next state code (R)
 *     +0x3D0 (u8)  action cooldown/timer (W = 3 or 8)
 *     +0x3D1 (u8)  cleared to 0 (W)
 *   Ball (Scratchpad+0x2F4):
 *     +0x002 (s16) x position (R)
 *     +0x006 (s16) z position (R)
 *     +0x00C (s32) y velocity (scaled by 736) (R)
 *     +0x3A0 (u32) speed << 8 (R)
 *     +0x3B0 (u8)  set to 4 or 10 (W)
 *   Globals:
 *     D_8010C32C (u32[]) player stat table indexed by side*264 + slot*12;
 *                        bits 12..15 used as skill stat (0..15) (R)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x6000 = 96 << 8 ball speed threshold for anim 1 fast deflection
 *   0x5000 = 80 << 8 ball speed threshold for standard deflection
 *   0x41   = 65/256 (~91.4 deg) half-cone angle threshold
 *   736    = ball vertical velocity scale divisor (matches func_8004B0F0)
 *   0x53..0x57 = 83..87 base power clamp upper/lower offsets
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_8010C32C[];

extern int func_8005C7A0(int);
extern void func_8005BA30(unsigned char*, unsigned int, unsigned int);
extern void func_8005BA78(unsigned char*, unsigned int, unsigned int);
extern unsigned int func_8001E6B8(unsigned int);
extern void func_8004B0F0(unsigned char*, unsigned int, unsigned int, int);
extern unsigned int func_8005B7CC(unsigned int, unsigned int, unsigned int);
extern void func_80054A1C(unsigned char*);
extern unsigned int func_8005B36C(int, int, int, int);

void func_80031268(unsigned char* actor) {
    int jitter;
    unsigned int targetAngle;
    unsigned char* ball;
    unsigned int angleToBall;
    unsigned int diff;
    int inCone;

    jitter = func_8005C7A0(3);
    B(actor, 0x3D1) = 0;
    targetAngle = B(actor, 0x3AB) + jitter;

    if (H(actor, 0x38E) == 1 && W(W(Scratchpad, 0x2F4), 0x3A0) >= 0x6001U) {
        unsigned int row = (unsigned int)B(actor, 0x392) * 12 + (unsigned int)B(actor, 0x398) * 264;
        unsigned int stat = (W(D_8010C32C, row) >> 12) & 0xF;
        unsigned int rnd = func_8001E6B8((9U - stat) << 1);
        unsigned int ballSpeedRaw = W(W(Scratchpad, 0x2F4), 0x3A0) >> 8;
        unsigned int power = (ballSpeedRaw * 3U) / 10U + rnd;
        unsigned int clampedAngle = targetAngle & 0xFF;

        func_8005BA30(actor, 0x16, 1);
        func_8004B0F0(actor, clampedAngle, power, 1);

        ball = (unsigned char*)W(Scratchpad, 0x2F4);
        B(Scratchpad, 0x2A9) = B(actor, 0x38D);
        B(ball, 0x3B0) = 4;
        S(Scratchpad, 0x31C) = B(actor, 0x399) ^ 1;
        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(clampedAngle, B(actor, 0x3AA), 12);
        B(actor, 0x3D0) = 3;
        B(actor, 0x397) = 0x15;
        return;
    }

    func_80054A1C(actor);
    ball = (unsigned char*)W(Scratchpad, 0x2F4);
    angleToBall = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));

    if (H(actor, 0x38E) != 1) {
        unsigned int bodyAngle = B(actor, 0x3AA);
        diff = (angleToBall - bodyAngle) & 0xFF;
        if (diff < 0x81) {
            inCone = (diff < 0x41); /* < ~91.4 deg */
        } else {
            inCone = (((bodyAngle - angleToBall) & 0xFF) < 0x41);
        }

        if (inCone) {
            if (W(actor, 0x3A0) < (unsigned int)(B(actor, 0x3B4) - 8)) {
                goto transition_state;
            }
            ball = (unsigned char*)W(Scratchpad, 0x2F4);
            if (W(ball, 0x3A0) < 0x5000U) {
                goto transition_state;
            }
            if (W(ball, 0x3A0) < 0x5001U) {
                unsigned int row = (unsigned int)B(actor, 0x392) * 12 + (unsigned int)B(actor, 0x398) * 264;
                unsigned int stat = (W(D_8010C32C, row) >> 12) & 0xF;
                if ((int)(9U - stat) < 2 || func_8001E6B8(7U - stat) == 0) {
                    goto transition_state;
                }
            }
        }

        {
            unsigned int row = (unsigned int)B(actor, 0x392) * 12 + (unsigned int)B(actor, 0x398) * 264;
            unsigned int stat = (W(D_8010C32C, row) >> 12) & 0xF;
            unsigned int rnd = func_8001E6B8((9U - stat) << 2);
            unsigned int ballSpeedScaled = W(W(Scratchpad, 0x2F4), 0x3A0);
            unsigned int power = (ballSpeedScaled * 7U) / 2560U + rnd; /* (speed_raw * 7) / 10 + rnd */
            unsigned int minPow = 0x53U - stat;
            unsigned int maxPow = 0x57U - stat;
            int lift;

            if (power < minPow) {
                power = minPow;
            } else if (maxPow < power) {
                power = maxPow;
            }

            lift = (-SW(W(Scratchpad, 0x2F4), 0xC)) / 736;
            func_8004B0F0(actor, targetAngle & 0xFF, power, lift);

            ball = (unsigned char*)W(Scratchpad, 0x2F4);
            B(Scratchpad, 0x2A9) = B(actor, 0x38D);
            B(ball, 0x3B0) = 10;
            S(Scratchpad, 0x31C) = B(actor, 0x399) ^ 1;

            bodyAngle = B(actor, 0x3AA);
            diff = (angleToBall - bodyAngle) & 0xFF;
            if (diff < 0x81) {
                if (diff < 0x41) {
                    goto transition_state;
                }
            } else if (((bodyAngle - angleToBall) & 0xFF) < 0x41) {
                goto transition_state;
            }

            func_8005BA30(actor, 0x2A, 0xF);
            B(actor, 0x3AA) = (unsigned char)func_8005B7CC(targetAngle & 0xFF, B(actor, 0x3AA), 12);
            B(actor, 0x3AC) = (((targetAngle - angleToBall) & 0xFF) < 0x81) ? 1 : 0;
            B(actor, 0x3D0) = 8;
            B(actor, 0x397) = 0x16;
            return;
        }
    }

transition_state:
    {
        int nextState = S(actor, 0x3CC);
        if (nextState == 4) {
            B(actor, 0x396) = 4;
            B(actor, 0x3B1) = 0;
            B(actor, 0x3B0) = 0;
            H(actor, 0x3C2) = 0;
            B(actor, 0x397) = 1;
        } else if (nextState == 0xFF) {
            unsigned char newState = 3;
            if (B(actor, 0x38D) == B(Scratchpad, 0x2A8)) {
                func_8005BA78(actor, 0x34, B(actor, 0x390));
                newState = 1;
            }
            B(actor, 0x396) = newState;
            B(actor, 0x397) = 0;
        } else {
            B(actor, 0x397) = 0;
            B(actor, 0x396) = (unsigned char)H(actor, 0x3CC);
        }
    }
}
