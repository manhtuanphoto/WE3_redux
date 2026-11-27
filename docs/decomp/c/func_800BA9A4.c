/*
 * func_800BA9A4
 *
 * Purpose:
 *   PsyQ hardware interrupt callback dispatcher and related callback management
 *   routines contained in the same assembly unit (func_800BA9A4 plus three
 *   adjacent unlabeled library routines: register callback, stop callbacks,
 *   and restart callbacks).
 *   func_800BA9A4 dispatches pending masked IRQ bits in D_800D4E78/D_800D4E7C
 *   to up to 11 registered callbacks in D_800D3DEC+4..0x2C, detects stuck IRQs
 *   after 0x801 (2049) iterations, and logs errors via func_800AD698.
 *
 * Arguments:
 *   func_800BA9A4: None (void)
 *   (Adjacent unlabeled routines take (index, callback) or (void))
 *
 * Return value:
 *   func_800BA9A4: None (void)
 *
 * Global fields read / written:
 *   D_800D3DEC (struct):
 *     +0x00 (u16) initialized/active flag
 *     +0x02 (u16) D_800D3DEE: inside-dispatcher flag
 *     +0x04..+0x2F (void (*[11])(void)) D_800D3DF0: IRQ callback table
 *     +0x30 (u16) D_800D3E1C: active IRQ enable mask
 *     +0x32 (u16) saved I_MASK on StopCallback
 *     +0x34 (u32) saved DMA DPCR on StopCallback
 *     +0x38 (u8[]) context buffer passed to func_800BAE38
 *   D_800D4E78 (volatile u16 **) pointer to I_STAT register
 *   D_800D4E7C (volatile u16 **) pointer to I_MASK register
 *   D_800D4E80 (volatile u32 **) pointer to DMA DPCR register
 *   D_800D4E84 (u32) stuck-IRQ timeout counter (limit 0x801 = 2049)
 *
 * Scratchpad (0x1F800000) fields read / written:
 *   None
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x0B (11) = number of IRQ callback slots
 *   0x801 (2049) = stuck-IRQ spin threshold before clearing I_STAT
 *   0x77777777 = mask applied to *D_800D4E80 when stopping callbacks
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_80015490;
extern unsigned char D_800154AC;
extern unsigned char D_800D3DEC[];
extern void (*D_800D3DF0[])(void);
extern unsigned short D_800D3DEE;
extern unsigned short D_800D3E1C;
extern volatile unsigned short *D_800D4E78;
extern volatile unsigned short *D_800D4E7C;
extern volatile unsigned int *D_800D4E80;
extern unsigned int D_800D4E84;

extern void func_800ACE68(void);
extern void func_800ACF08(int disable);
extern void func_800AD3D8(void);
extern void func_800AD698(unsigned char *fmt, ...);
extern void func_800BA728(int channel, int disable);
extern void func_800BAE18(void);
extern void func_800BAE28(void);
extern void func_800BAE38(unsigned char *buf);

void func_800BA9A4(void)
{
    unsigned int pending;
    int irq;
    void (**cb)(void);

    if (H(D_800D3DEC, 0) == 0) {
        func_800AD698(&D_80015490, *D_800D4E78);
        func_800BAE18();
    }

    H(D_800D3DEC, 2) = 1;
    pending = H(D_800D3DEC, 0x30) & (*D_800D4E78) & (*D_800D4E7C);

    while (pending != 0) {
        irq = 0;
        cb = (void (**)(void))(D_800D3DEC + 4);
        while (pending != 0 && irq < 11) {
            if (pending & 1) {
                *D_800D4E78 = (unsigned short)~(1 << irq);
                if (*cb != 0) {
                    (*cb)();
                }
            }
            cb++;
            pending = (pending >> 1) & 0xFFFF;
            irq++;
        }
        pending = D_800D3E1C & (*D_800D4E78) & (*D_800D4E7C);
    }

    if ((*D_800D4E78) & (*D_800D4E7C)) {
        if ((int)D_800D4E84++ >= 0x801) {
            func_800AD698(&D_800154AC, *D_800D4E78, *D_800D4E7C);
            D_800D4E84 = 0;
            *D_800D4E78 = 0;
        }
    } else {
        D_800D4E84 = 0;
    }

    D_800D3DEE = 0;
    func_800BAE18();
}

/* Adjacent unlabeled function in func_800BA9A4.s (InterruptCallback) */
static void (*func_800BA9A4_setCallback(int irq, void (*callback)(void)))(void)
{
    void (**slot)(void) = &D_800D3DF0[irq];
    void (*prev)(void) = *slot;
    unsigned short mask;

    if (callback == prev) {
        return prev;
    }
    if (H(D_800D3DEC, 0) == 0) {
        return prev;
    }

    mask = *D_800D4E7C;
    *D_800D4E7C = 0;

    if (callback != 0) {
        *slot = callback;
        mask |= (unsigned short)(1 << irq);
        H(D_800D3DEC, 0x30) |= (unsigned short)(1 << irq);
    } else {
        *slot = 0;
        mask &= (unsigned short)~(1 << irq);
        H((unsigned char *)D_800D3DF0, 0x2C) &= (unsigned short)~(1 << irq);
    }

    if (irq == 0) {
        int disable = (callback == 0);
        func_800ACF08(disable);
        func_800BA728(3, disable);
    }
    if (irq == 4) {
        func_800BA728(0, callback == 0);
    }
    if (irq == 5) {
        func_800BA728(1, callback == 0);
    }
    if (irq == 6) {
        func_800BA728(2, callback == 0);
    }

    *D_800D4E7C = mask;
    return prev;
}

/* Adjacent unlabeled function in func_800BA9A4.s (StopCallback) */
static unsigned char *func_800BA9A4_stopCallbacks(void)
{
    if (H(D_800D3DEC, 0) == 0) {
        return 0;
    }

    func_800AD3D8();
    H(D_800D3DEC, 0x32) = *D_800D4E7C;
    W(D_800D3DEC, 0x34) = *D_800D4E80;
    *D_800D4E7C = 0;
    *D_800D4E78 = *D_800D4E7C;
    *D_800D4E80 &= 0x77777777;
    func_800BAE28();
    H(D_800D3DEC, 0) = 0;
    return D_800D3DEC;
}

/* Adjacent unlabeled function in func_800BA9A4.s (RestartCallback) */
static unsigned char *func_800BA9A4_restartCallbacks(void)
{
    if (H(D_800D3DEC, 0) != 0) {
        return 0;
    }

    func_800BAE38(D_800D3DEC + 0x38);
    H(D_800D3DEC, 0) = 1;
    *D_800D4E7C = H(D_800D3DEC, 0x32);
    *D_800D4E80 = W(D_800D3DEC, 0x34);
    func_800ACE68();
    return D_800D3DEC;
}
