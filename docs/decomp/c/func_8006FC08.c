/*
 * func_8006FC08 - Rotate/cycle a contiguous range of colors in a 16-color VRAM CLUT
 *
 * Purpose:
 *   Reads a 16x1 CLUT line from VRAM at coordinates stored in D_800FF748+0x7FA4/
 *   0x7FA6 for `slot`, shifts entries [idx_from + 1 .. idx_to] down by one index,
 *   places the original `idx_from` entry at `idx_to`, and uploads the modified
 *   16-color CLUT back to VRAM.
 *
 * Arguments:
 *   slot (unsigned int):     CLUT slot index (masked to 8 bits)
 *   idx_from (unsigned int): starting CLUT index to rotate out (masked to 8 bits)
 *   idx_to (unsigned int):   ending CLUT index receiving `clut[idx_from]` (masked to 8 bits)
 *
 * Return value:
 *   None (void)
 *
 * Globals read:
 *   D_800FF748 (u8[]): VRAM coordinate table (+0x7FA6 = X, +0x7FA4 = Y per 4-byte slot)
 */

#define H(p, o) (*(unsigned short *)((p) + (o)))

extern unsigned char D_800FF748[];

extern void func_800AE840(unsigned short *rect, unsigned short *dst); /* StoreImage */
extern void func_800AE7E0(unsigned short *rect, unsigned short *src); /* LoadImage */
extern void func_800AE534(int mode);                                  /* DrawSync */

void func_8006FC08(unsigned int slot, unsigned int idx_from, unsigned int idx_to)
{
    unsigned short rect[4];
    unsigned short clut[16];
    unsigned char *entry;
    unsigned short saved;
    unsigned int dst_idx;
    unsigned int src_idx;
    unsigned int end_idx;

    entry = D_800FF748 + ((slot & 0xFFu) * 4u);
    rect[0] = H(entry, 0x7FA6);
    rect[1] = H(entry, 0x7FA4);
    rect[2] = 0x10;
    rect[3] = 1;

    func_800AE840(rect, clut);
    func_800AE534(0);

    dst_idx = idx_from + 1u;
    src_idx = dst_idx & 0xFFu;
    saved = clut[idx_from & 0xFFu];
    end_idx = idx_to & 0xFFu;

    while (src_idx <= end_idx) {
        clut[src_idx - 1u] = clut[src_idx];
        dst_idx += 1u;
        src_idx = dst_idx & 0xFFu;
    }

    clut[end_idx] = saved;

    func_800AE7E0(rect, clut);
    func_800AE534(0);
}
