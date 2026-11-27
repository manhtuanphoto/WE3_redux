/*
 * func_800AFAE4
 *
 * Purpose:
 *   Clears or fills a VRAM rectangle (PS1 ClearImage-style helper). Clamps
 *   rect->w and rect->h to [0, D_800CEAC8 - 1] and [0, D_800CEACA - 1]. If both
 *   rect->x and rect->w are 64-pixel aligned (& 0x3F == 0), emits a fast 0x02
 *   VRAM Fill Rectangle packet in D_800E7048; otherwise emits a 0x60 TILE packet
 *   bracketed by full-VRAM draw-area/offset setup and restore packets
 *   (D_800E7048 linked to D_800E7070), then submits via func_800B0248.
 *
 * Arguments:
 *   rect (short *): pointer to 4 signed 16-bit values { x, y, w, h }
 *   color (unsigned int): 24-bit BGR fill color in bits 0..23; bit 31 sets dfe/display-area bit
 *
 * Return value:
 *   Always returns 0.
 *
 * Global fields read:
 *   D_800CEAC8 (s16/u16) VRAM width limit,
 *   D_800CEACA (s16/u16) VRAM height limit,
 *   D_800CEB9C (u32 *) pointer to current GPU texpage/mode word
 *
 * Global fields written:
 *   D_800E7048 (u32[]) primary GPU DMA packet buffer,
 *   D_800E7070 (u32[]) secondary GPU DMA packet buffer (restore commands)
 */

#define H(p, o) (*(unsigned short *)((p) + (o)))
#define S(p, o) (*(signed short *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

extern signed short D_800CEAC8;
extern signed short D_800CEACA;
extern unsigned int *D_800CEB9C;
extern unsigned char D_800E7048[];
extern unsigned char D_800E7070[];

extern unsigned int func_800B0290(int idx);
extern void func_800B0248(unsigned char *pkt);

int func_800AFAE4(unsigned char *rect, unsigned int color)
{
    signed short w_s = S(rect, 4);
    unsigned short w_u = H(rect, 4);
    signed short h_s;
    unsigned short h_u;
    unsigned short clamped_w;
    unsigned short clamped_h;

    if (w_s < 0) {
        clamped_w = 0;
    } else if ((int)D_800CEAC8 - 1 < (int)w_s) {
        clamped_w = (unsigned short)((unsigned short)D_800CEAC8 - 1);
    } else {
        clamped_w = w_u;
    }
    H(rect, 4) = clamped_w;

    h_s = S(rect, 6);
    h_u = H(rect, 6);
    if (h_s < 0) {
        clamped_h = 0;
    } else if ((int)D_800CEACA - 1 < (int)h_s) {
        clamped_h = (unsigned short)((unsigned short)D_800CEACA - 1);
    } else {
        clamped_h = h_u;
    }
    H(rect, 6) = clamped_h;

    if ((H(rect, 0) & 0x3F) != 0 || (H(rect, 4) & 0x3F) != 0) {
        unsigned int tpage_bits = (*D_800CEB9C & 0x7FFU) | ((color >> 31) << 10) | 0xE1000000U;
        W(D_800E7048, 0x00) = ((unsigned int)D_800E7070 & 0x00FFFFFFU) | 0x08000000U;
        W(D_800E7048, 0x04) = 0xE3000000U;
        W(D_800E7048, 0x08) = 0xE4FFFFFFU;
        W(D_800E7048, 0x0C) = 0xE5000000U;
        W(D_800E7048, 0x10) = 0xE6000000U;
        W(D_800E7048, 0x14) = tpage_bits;
        W(D_800E7048, 0x18) = (color & 0x00FFFFFFU) | 0x60000000U;
        W(D_800E7048, 0x1C) = W(rect, 0x00);
        W(D_800E7048, 0x20) = W(rect, 0x04);

        W(D_800E7070, 0x00) = 0x03FFFFFFU;
        W(D_800E7070, 0x04) = func_800B0290(3) | 0xE3000000U;
        W(D_800E7070, 0x08) = func_800B0290(4) | 0xE4000000U;
        W(D_800E7070, 0x0C) = func_800B0290(5) | 0xE5000000U;
    } else {
        unsigned int tpage_bits = (*D_800CEB9C & 0x7FFU) | ((color >> 31) << 10) | 0xE1000000U;
        W(D_800E7048, 0x00) = 0x05FFFFFFU;
        W(D_800E7048, 0x04) = 0xE6000000U;
        W(D_800E7048, 0x08) = tpage_bits;
        W(D_800E7048, 0x0C) = (color & 0x00FFFFFFU) | 0x02000000U;
        W(D_800E7048, 0x10) = W(rect, 0x00);
        W(D_800E7048, 0x14) = W(rect, 0x04);
    }

    func_800B0248(D_800E7048);
    return 0;
}
