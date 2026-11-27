/*
 * func_800AE8A0
 *
 * Purpose:
 *   Enqueues a VRAM-to-VRAM rectangular copy command (PsyQ MoveImage
 *   equivalent). Validates the source rect via func_800AE59C, checks that width
 *   (+0x04) and height (+0x06) are non-zero, packs the source rect and target
 *   coordinates into D_800CEB64, and dispatches through D_800CEABC->+0x08.
 *
 * Arguments:
 *   rect (unsigned char *) - source RECT (+0x00 x, +0x02 y, +0x04 s16 w, +0x06 s16 h)
 *   dstX (int)             - destination VRAM X coordinate (low 16 bits)
 *   dstY (int)             - destination VRAM Y coordinate (low 16 bits)
 *
 * Return value:
 *   (int) -1 if width or height is 0; otherwise the return value of D_800CEABC->+0x08.
 *
 * Global fields read/written:
 *   D_80014F6C (const char[] / label) passed to func_800AE59C
 *   D_800CEB64 (u32[3]) GPU command payload buffer (header precedes at D_800CEB64 - 8)
 *   D_800CEABC (u32 *) GPU dispatch table (+0x08 fn ptr, +0x18 context/arg0)
 */

#define S(p, o) (*(signed short *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned char D_80014F6C[];
extern unsigned char D_800CEB64[];
extern unsigned char *D_800CEABC;

extern void func_800AE59C(unsigned char *tag, unsigned char *rect);

typedef int (*gpu_dispatch_fn_t)(unsigned int ctx, unsigned char *pkt, unsigned int size, unsigned int flags);

int func_800AE8A0(unsigned char *rect, int dstX, int dstY)
{
    unsigned int packedDst;
    gpu_dispatch_fn_t dispatchFn;

    func_800AE59C(D_80014F6C, rect);

    if (S(rect, 0x04) == 0 || S(rect, 0x06) == 0) {
        return -1;
    }

    packedDst = ((unsigned int)dstY << 16) | ((unsigned int)dstX & 0xFFFFU);
    W(D_800CEB64, 0x00) = W(rect, 0x00);
    W(D_800CEB64, 0x04) = packedDst;
    W(D_800CEB64, 0x08) = W(rect, 0x04);

    dispatchFn = (gpu_dispatch_fn_t)W(D_800CEABC, 0x08);
    return dispatchFn(W(D_800CEABC, 0x18), D_800CEB64 - 8, 0x14U, 0U);
}
