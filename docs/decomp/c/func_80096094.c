/*
 * func_80096094
 *
 * Purpose:
 *   Updates the cut-in / foul / set-piece focus camera state machine.
 *   When arg0 is 1 or 2, updates the active camera behind the ball or restores
 *   the gameplay camera when Scratchpad+0x309 == 2. Otherwise runs a 4-state
 *   transition (Scratchpad+0x2F0 = 0..3): snapshot/initialize focus camera on
 *   actor Scratchpad+0x2AD, hold 5 frames, interpolate 24 frames toward the
 *   target camera, and interpolate 10 frames back to the match camera.
 *
 * Arguments:
 *   mode - signed integer camera mode (1..2 = live ball/restore mode;
 *          otherwise runs the Scratchpad+0x2F0 transition state machine).
 *
 * Return value:
 *   None (void).
 *
 * Actor fields read/written:
 *   Focus actor (D_800FF748 + (Scratchpad[0x2AD] % 24) * 1000):
 *     Reads: +0x02 (x), +0x06 (z)
 *   Ball actor (Scratchpad + 0x2F4):
 *     Reads: +0x02 (x), +0x06 (z), +0x3A4 (move heading)
 *
 * Scratchpad fields read/written:
 *   Reads:  0x1F0..0x20C (current camera params, 8 s32 words),
 *           0x210..0x22C (saved/match camera params, 8 s32 words),
 *           0x2AD (focus actor id), 0x2E7 (camera yaw angle),
 *           0x2E8 (camera transition frame timer), 0x2F0 (camera state),
 *           0x2F4 (ball actor ptr), 0x309 (camera restore/active flag)
 *   Writes: 0x1F0..0x204 (eye xyz & look-at xyz), 0x210..0x22C (saved camera),
 *           0x2E8 (frame timer), 0x2F0 (camera state -> 1, 2, 3, 0xFF),
 *           0x2F2 (camera mode = 3), 0x309 (= 1 or 0xFF)
 *
 * Magic numbers / unit conversions (250 raw = 1 m, 30 frames/s):
 *   5 frames  = 0.167 s pre-pan hold (state 1)
 *   24 frames = 0.800 s pan-in duration (state 2)
 *   10 frames = 0.333 s pan-out duration (state 3)
 *   -0x480 / -0x400 = -1152 / -1024 raw (4.608 / 4.096 m camera eye height)
 *   -0x200 / -0x1C0 = -512 / -448 raw (2.048 / 1.792 m camera look-at height)
 *   0x800  = 2048 raw (8.192 m look-at distance ahead of ball)
 *   0xC00  = 3072 raw (12.288 m focus camera distance behind actor)
 *   0x1800 = 6144 raw (24.576 m camera distance behind ball)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern signed int D_800E6D70[];

extern void func_80051A84(unsigned char *cam);
extern int func_8001E694(int angle, int scale);
extern int func_8001E5A4(int angle, int scale);

void func_80096094(int mode)
{
    unsigned char focus_id = B(Scratchpad, 0x2AD);
    unsigned char *focus_actor = D_800FF748 + (focus_id % 24U) * 1000;
    int i;

    if (mode > 0 && mode < 3) {
        unsigned char flag = B(Scratchpad, 0x309);
        if (flag == 0xFF) {
            return;
        }
        if (flag == 2) {
            B(Scratchpad, 0x2F2) = 3;
            func_80051A84(Scratchpad + 0x1F0);
            for (i = 0; i < 8; i++) {
                W(Scratchpad, 0x210 + i * 4) = W(Scratchpad, 0x1F0 + i * 4);
            }
            B(Scratchpad, 0x309) = 0xFF;
            return;
        }
        {
            unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
            int cos_near = func_8001E694(B(ball, 0x3A4), 0x800);
            int sin_near;
            int cos_far;
            int sin_far;

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            SW(Scratchpad, 0x200) = -0x200;
            SW(Scratchpad, 0x1FC) = S(ball, 0x02) + cos_near;
            sin_near = func_8001E5A4(B(ball, 0x3A4), 0x800);

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            SW(Scratchpad, 0x204) = S(ball, 0x06) + sin_near;
            cos_far = func_8001E694(B(ball, 0x3A4), 0x1800);

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            SW(Scratchpad, 0x1F4) = -0x480;
            SW(Scratchpad, 0x1F0) = S(ball, 0x02) - cos_far;
            sin_far = func_8001E5A4(B(ball, 0x3A4), 0x1800);

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            SW(Scratchpad, 0x1F8) = S(ball, 0x06) - sin_far;
        }
        return;
    }

    switch (B(Scratchpad, 0x2F0)) {
    case 0:
        S(Scratchpad, 0x2E8) = 0;
        if (B(Scratchpad, 0x309) != 0) {
            B(Scratchpad, 0x2F0) = 3;
            return;
        }
        B(Scratchpad, 0x2F2) = 3;
        func_80051A84(Scratchpad + 0x210);
        SW(Scratchpad, 0x200) = -0x1C0;
        SW(Scratchpad, 0x1FC) = S(focus_actor, 0x02);
        SW(Scratchpad, 0x204) = S(focus_actor, 0x06);
        {
            int cos_v = func_8001E694(B(Scratchpad, 0x2E7), 0xC00);
            int sin_v;
            SW(Scratchpad, 0x1F4) = -0x400;
            SW(Scratchpad, 0x1F0) = S(focus_actor, 0x02) - cos_v;
            sin_v = func_8001E5A4(B(Scratchpad, 0x2E7), 0xC00);
            SW(Scratchpad, 0x1F8) = S(focus_actor, 0x06) - sin_v;
        }
        for (i = 0; i < 8; i++) {
            D_800E6D70[i] = SW(Scratchpad, 0x1F0 + i * 4);
        }
        B(Scratchpad, 0x2F0) = (unsigned char)(B(Scratchpad, 0x2F0) + 1);
        break;

    case 1: {
        int t = S(Scratchpad, 0x2E8) + 1;
        if ((signed short)t < 5) {
            S(Scratchpad, 0x2E8) = (signed short)t;
        } else {
            S(Scratchpad, 0x2E8) = 0;
            B(Scratchpad, 0x2F0) = (unsigned char)(B(Scratchpad, 0x2F0) + 1);
        }
        break;
    }

    case 2: {
        int t = S(Scratchpad, 0x2E8);
        int rem = 24 - t;
        int next_t;

        for (i = 0; i < 6; i++) {
            SW(Scratchpad, 0x1F0 + i * 4) =
                (D_800E6D70[i] * rem + SW(Scratchpad, 0x210 + i * 4) * t) / 24;
        }
        next_t = S(Scratchpad, 0x2E8) + 1;
        S(Scratchpad, 0x2E8) = (signed short)next_t;
        if ((signed short)next_t >= 24) {
            B(Scratchpad, 0x309) = 1;
            for (i = 0; i < 8; i++) {
                W(Scratchpad, 0x210 + i * 4) = W(Scratchpad, 0x1F0 + i * 4);
            }
            B(Scratchpad, 0x2F0) = 0xFF;
        }
        break;
    }

    case 3: {
        int t;
        int rem;

        B(Scratchpad, 0x2F2) = 3;
        func_80051A84(Scratchpad + 0x1F0);
        t = S(Scratchpad, 0x2E8);
        if (t >= 10) {
            B(Scratchpad, 0x2F0) = 0xFF;
            return;
        }
        rem = 10 - t;
        for (i = 0; i < 6; i++) {
            SW(Scratchpad, 0x1F0 + i * 4) =
                (SW(Scratchpad, 0x210 + i * 4) * rem + SW(Scratchpad, 0x1F0 + i * 4) * t) / 10;
        }
        S(Scratchpad, 0x2E8) = (signed short)(S(Scratchpad, 0x2E8) + 1);
        break;
    }

    default:
        break;
    }
}
