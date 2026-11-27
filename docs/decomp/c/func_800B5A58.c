/*
 * func_800B5A58
 *
 * Purpose:
 *   Initializes a rendering/ordering-table context struct: stores 16-bit
 *   dimensions/coordinates at +0x08/+0x0C, computes the end-of-OT pointer
 *   `base + (4 << shift) - 4` at +0x10, and clears/initializes the OT buffer
 *   of `(1 << shift)` entries via func_800AEA20.
 *
 * Arguments:
 *   w   (unsigned int):  masked to 16 bits and stored at ctx+0x08
 *   h   (unsigned int):  masked to 16 bits and stored at ctx+0x0C
 *   ctx (unsigned char *): context struct:
 *     +0x00 (u32) shift (log2 entry count)
 *     +0x04 (u32) pointer to ordering table buffer
 *     +0x08 (u32) width / x parameter (written)
 *     +0x0C (u32) height / y parameter (written)
 *     +0x10 (u32) pointer to last word of ordering table (written)
 *
 * Return value:
 *   None (void)
 */

#define W(p, o) (*(unsigned int *)((p) + (o)))

extern void func_800AEA20(unsigned char *otBase, unsigned int count);

void func_800B5A58(unsigned int w, unsigned int h, unsigned char *ctx)
{
    unsigned int shift;
    unsigned int otBase;

    W(ctx, 0x08) = w & 0xFFFFU;
    shift = W(ctx, 0x00);
    otBase = W(ctx, 0x04);
    W(ctx, 0x0C) = h & 0xFFFFU;
    W(ctx, 0x10) = otBase + (4U << shift) - 4U;

    func_800AEA20((unsigned char *)W(ctx, 0x04), 1U << W(ctx, 0x00));
}
