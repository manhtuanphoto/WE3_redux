/*
 * func_8006FCFC
 *
 * Purpose:
 *   Rotates colors 1..15 of a 16-entry (16x1) VRAM CLUT (palette) by one step
 *   (preserving entry 0; shifting entries 2..15 down to 1..14 and wrapping
 *   entry 1 to entry 15). Reads the VRAM CLUT coordinates from
 *   `D_800FF748 + 0x7FA4 + (idx & 0xFF) * 4`, downloads via `func_800AE840`
 *   (StoreImage) + `func_800AE534(0)` (DrawSync), rotates, and uploads back
 *   via `func_800AE7E0` (LoadImage) + `func_800AE534(0)`.
 *
 * Arguments:
 *   idx (unsigned int) - palette slot index (low 8 bits)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800FF748 + 0x7FA4 + (idx & 0xFF) * 4:
 *     +0x00 (u16) VRAM CLUT y
 *     +0x02 (u16) VRAM CLUT x
 */

#define H(p, o) (*(unsigned short *)((p) + (o)))

extern unsigned char D_800FF748[];

extern void func_800AE840(unsigned short *rect, unsigned short *buf);
extern void func_800AE7E0(unsigned short *rect, unsigned short *buf);
extern void func_800AE534(unsigned int mode);

void func_8006FCFC(unsigned int idx)
{
    unsigned short rect[4];
    unsigned short clut[16];
    unsigned char *entry = D_800FF748 + ((idx & 0xFFU) << 2);
    unsigned short savedColor1;
    unsigned int i;

    rect[0] = H(entry, 0x7FA6);
    rect[1] = H(entry, 0x7FA4);
    rect[2] = 0x10;
    rect[3] = 1;

    func_800AE840(rect, clut);
    func_800AE534(0U);

    savedColor1 = clut[1];
    for (i = 2U; i <= 15U; i++) {
        clut[i - 1U] = clut[i];
    }
    clut[15] = savedColor1;

    func_800AE7E0(rect, clut);
    func_800AE534(0U);
}
