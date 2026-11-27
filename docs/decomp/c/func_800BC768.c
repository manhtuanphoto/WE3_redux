/*
 * func_800BC768 (plus contiguous tail helper sub_800BC99C)
 *
 * Purpose:
 *   PsyQ VSync / root-counter event configuration helper. Waits briefly, inspects
 *   mode word D_800D4F4C (D_800D4F5C - 0x10) and divider D_800D4F50 to select a
 *   root counter (0xF2000002 or 0xF2000003) and target count (0x44E8, 0x89D0,
 *   1, or 0x204CC0/rate, 0x409980/rate), then installs or enables the hardware
 *   event/callback via func_800AD050 / func_800ACF18 / func_800BA768 / func_800BA7C8.
 *   sub_800BC99C calls func_800BC768(1).
 *
 * Arguments:
 *   a0 (u32): flag used when mode D_800D4F4C == 5 (0 = event-only, non-zero = RCnt 0xF2000003)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800D4F4C (s32, D_800D4F5C - 0x10) mode/rate selector
 *   D_800D4F50 (u32) enable/override flag
 *   D_800D4F54 (u32, D_800D4F5C - 0x08) saved event descriptor
 *   D_800D4F58 (u32, D_800D4F5C - 0x04) event handle returned by func_800BA768
 *   D_800D4F5C (u8[3]) state bytes: +0 active, +1 alt-callback flag, +2 mode/channel (0, 6, 0x7F)
 *   D_800BC9D8 / D_800BCA24 handler callbacks
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800D4F5C[];
extern unsigned int  D_800D4F50;
extern unsigned char D_800BC9D8[];
extern unsigned char D_800BCA24[];

extern void func_800AD3D8(void);
extern void func_800BA7C8(unsigned int event);
extern void func_800AD050(unsigned int rcnt);
extern void func_800ACF18(unsigned int rcnt, unsigned short target, unsigned int mode);
extern unsigned int func_800BA768(signed int spec, void *cb);
extern void func_800ACE68(void);

void func_800BC768(unsigned int a0)
{
    volatile int delay = 998;
    unsigned int rcnt = 0xF2000002U;
    unsigned int target = 0x44E8U;
    signed int mode;

    while (delay >= 0) {
        delay--;
    }

    mode = SW(D_800D4F5C, -0x10);
    B(D_800D4F5C, 2) = 6;
    B(D_800D4F5C, 0) = 0;
    B(D_800D4F5C, 1) = 0;
    W(D_800D4F5C, -4) = 0;

    if (mode == 2) {
        /* Keep default rcnt = 0xF2000002, target = 0x44E8 */
    } else if (mode < 3) {
        if (mode == 0) {
            B(D_800D4F5C, 2) = 0x7F;
            return;
        }
        goto calc_rate;
    } else if (mode == 3) {
        target = 0x89D0U;
    } else if (mode == 5) {
        B(D_800D4F5C, 2) = 0;
        if (a0 == 0) {
            B(D_800D4F5C, 0) = 1;
        } else {
            rcnt = 0xF2000003U;
            target = 1U;
        }
    } else {
        signed int div_val;
    calc_rate:
        if (D_800D4F50 != 0) {
            return;
        }
        div_val = SW(D_800D4F5C, -0x10);
        if (div_val < 0x46) {
            target = (unsigned int)(0x204CC0 / div_val);
            B(D_800D4F5C, 1) = (unsigned char)(B(D_800D4F5C, 1) + 1U);
        } else {
            target = (unsigned int)(0x409980 / div_val);
        }
    }

    if ((signed char)B(D_800D4F5C, 0) != 0) {
        func_800AD3D8();
        func_800BA7C8(W(D_800D4F5C, -8));
    } else {
        func_800AD3D8();
        func_800AD050(rcnt);
        func_800ACF18(rcnt, (unsigned short)target, 0x1000U);
        if ((signed char)B(D_800D4F5C, 2) == 0) {
            unsigned int handle = func_800BA768(0, 0);
            W(D_800D4F5C, -4) = handle;
            func_800BA768((signed char)B(D_800D4F5C, 2), D_800BC9D8);
        } else {
            void *cb = ((signed char)B(D_800D4F5C, 1) != 0)
                ? (void *)D_800BCA24
                : (void *)W(D_800D4F5C, -8);
            func_800BA768((signed char)B(D_800D4F5C, 2), cb);
        }
    }

    func_800ACE68();
}

void sub_800BC99C(void)
{
    func_800BC768(1);
}
