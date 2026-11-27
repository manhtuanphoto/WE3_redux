/*
 * func_800BC9B8 (plus contiguous callback labels D_800BC9D8 and D_800BCA24)
 *
 * Purpose:
 *   - func_800BC9B8: Calls func_800BC768(0) to initialize/reset VSync / root
 *     counter timing without forcing RCnt3.
 *   - D_800BC9D8: VSync/counter interrupt callback wrapper. Calls D_800D4F58()
 *     if non-NULL, then unconditionally calls D_800D4F54() (D_800D4F58 - 4).
 *   - D_800BCA24: Alternate counter callback wrapper. When D_800D4F60 == 0,
 *     sets D_800D4F60 = 1; otherwise clears D_800D4F60 = 0 and calls D_800D4F54().
 *
 * Arguments:
 *   None (void) for all three entry points.
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800D4F58 (void (*)(void)) optional pre-callback (at D_800D4F58 + 0x00)
 *   D_800D4F54 (void (*)(void)) main callback (at D_800D4F58 - 0x04)
 *   D_800D4F60 (u32)            toggle/skip flag for D_800BCA24
 */

#define W(p, o) (*(unsigned int *)((p) + (o)))

typedef void (*vsync_cb_t)(void);

extern unsigned char D_800D4F58[];
extern vsync_cb_t D_800D4F54;
extern unsigned int D_800D4F60;

extern void func_800BC768(unsigned int a0);

void func_800BC9B8(void)
{
    func_800BC768(0U);
}

void D_800BC9D8(void)
{
    vsync_cb_t preCb = (vsync_cb_t)W(D_800D4F58, 0x00);
    vsync_cb_t mainCb;

    if (preCb != 0) {
        preCb();
    }

    mainCb = (vsync_cb_t)W(D_800D4F58, -0x04);
    mainCb();
}

void D_800BCA24(void)
{
    if (D_800D4F60 == 0U) {
        D_800D4F60 = 1U;
    } else {
        vsync_cb_t mainCb = D_800D4F54;
        D_800D4F60 = 0U;
        mainCb();
    }
}
