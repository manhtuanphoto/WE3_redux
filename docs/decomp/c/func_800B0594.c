/*
 * func_800B0594
 *
 * Purpose:
 *   Flushes the GPU command/DMA queue and waits for completion (PsyQ libgpu
 *   DrawSync(0) / queue drain). If GPU status bit 0x01000000 is set, returns 1
 *   immediately; otherwise enters critical section via func_800BA8B4(0),
 *   drains 96-byte (0x60) queue entries from D_800CEBC0 to D_800CEBBC while
 *   polling GPUSTAT bit 26 (0x04000000 via *D_800CEB9C), exits critical section,
 *   fires completion callback D_800CEAD0 if D_800CEACC is set, and returns
 *   remaining queue count.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   1 if *D_800CEBA8 & 0x01000000 was set on entry;
 *   otherwise (D_800CEBBC - D_800CEBC0) & 0x3F (0 when fully drained).
 *
 * Global fields read:
 *   D_800CEBA8 (u32 *) GPU control/status flag pointer (bit 24 = 0x01000000),
 *   D_800CEB9C (u32 *) GPUSTAT register pointer (bit 26 = 0x04000000 DMA ready),
 *   D_800CEBBC (u32) queue write index,
 *   D_800CEBC0 (u32) queue read index,
 *   D_800CEBC8 (u32) saved critical-section token,
 *   D_800CEACC (u32) callback pending flag,
 *   D_800CEAD0 (u32) callback function pointer,
 *   D_8010A9B8 / D_8010A9BC / D_8010A9C0: 96-byte (0x60) queue entry array (fn, arg0, arg1)
 *
 * Global fields written:
 *   D_800CEBC8 (u32) = 1,
 *   D_800CEBC0 (u32) incremented mod 64,
 *   D_800CEACC (u32) cleared to 0 before invoking D_800CEAD0
 */

#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned int *D_800CEBA8;
extern unsigned int *D_800CEB9C;
extern unsigned int  D_800CEBBC;
extern unsigned int  D_800CEBC0;
extern unsigned int  D_800CEBC8;
extern unsigned int  D_800CEACC;
extern unsigned int  D_800CEAD0;
extern unsigned char D_8010A9B8[]; /* stride 0x60: +0x00 fn, +0x04 arg0, +0x08 arg1 */

extern void func_800BA8B4(unsigned int arg);
extern void func_800BA798(unsigned int a0, unsigned int a1);

typedef void (*gpu_queue_fn_t)(unsigned int arg0, unsigned int arg1);
typedef void (*gpu_sync_cb_t)(void);

unsigned int func_800B0594(void)
{
    if ((*D_800CEBA8 & 0x01000000U) != 0) {
        return 1;
    }

    func_800BA8B4(0);
    D_800CEBC8 = 1;

    while (D_800CEBBC != D_800CEBC0 && (*D_800CEBA8 & 0x01000000U) == 0) {
        unsigned int idx;
        unsigned int off;
        gpu_queue_fn_t fn;

        if (((D_800CEBC0 + 1U) & 0x3FU) != D_800CEBBC && D_800CEAD0 == 0) {
            func_800BA798(2, 0);
        }

        while ((*D_800CEB9C & 0x04000000U) == 0) {
            /* wait for GPUSTAT bit 26 (ready for DMA/command) */
        }

        idx = D_800CEBC0;
        off = idx * 0x60U;
        fn = (gpu_queue_fn_t)W(D_8010A9B8, off + 0x00);
        fn(W(D_8010A9B8, off + 0x04), W(D_8010A9B8, off + 0x08));

        D_800CEBC0 = (D_800CEBC0 + 1U) & 0x3FU;
    }

    func_800BA8B4(D_800CEBC8);

    if (D_800CEBBC == D_800CEBC0 && (*D_800CEBA8 & 0x01000000U) == 0) {
        if (D_800CEACC != 0 && D_800CEAD0 != 0) {
            gpu_sync_cb_t cb = (gpu_sync_cb_t)D_800CEAD0;
            D_800CEACC = 0;
            cb();
        }
    }

    return (D_800CEBBC - D_800CEBC0) & 0x3FU;
}
