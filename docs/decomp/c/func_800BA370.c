/*
 * func_800BA370
 *
 * Purpose:
 *   Waits for a PS1 DMA channel to become idle (logging via func_800AD698 on
 *   timeout after 0x10000 polls), updates a channel bit in *D_800D3D40 + 2,
 *   enables the channel in DPCR (*D_800D3D3C), programs MADR/BCR, waits for
 *   bit 6 of *D_800D3D24, and starts the DMA transfer via CHCR.
 *
 * Arguments:
 *   ch:     (int) PS1 DMA channel index (0..6)
 *   madr:   (unsigned int) DMA base address (MADR)
 *   ba:     (unsigned int) block count / upper 16 bits of BCR
 *   bs:     (unsigned int) block size / lower 16 bits of BCR
 *   chcr:   (unsigned int) DMA channel control value (CHCR)
 *   active: (unsigned char) if 1, sets channel bit in *D_800D3D40+2; else clears
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   PS1 DMA Channel Registers (0x1F801080 + ch * 0x10):
 *     +0x00 MADR (W), +0x04 BCR (W), +0x08 CHCR (R/W)
 *   D_800D3D24: (volatile u8**) pointer to hardware status register (bit 0x40 polled) (R)
 *   D_800D3D3C: (volatile u32**) pointer to DMA DPCR (0x1F8010F0) (R/W)
 *   D_800D3D40: (volatile u8**) pointer to DMA control/DICR-like block (R/W)
 *   D_8001542C: format string for DMA timeout diagnostic (R)
 */

#define DMA_MADR(ch) (*(volatile unsigned int*)(0x1F801080U + ((ch) << 4)))
#define DMA_BCR(ch)  (*(volatile unsigned int*)(0x1F801084U + ((ch) << 4)))
#define DMA_CHCR(ch) (*(volatile unsigned int*)(0x1F801088U + ((ch) << 4)))

extern unsigned char D_8001542C[];
extern volatile unsigned char *D_800D3D24;
extern volatile unsigned int *D_800D3D3C;
extern volatile unsigned char *D_800D3D40;

extern void func_800AD698(void *fmt, ...);

void func_800BA370(int ch, unsigned int madr, unsigned int ba, unsigned int bs,
                   unsigned int chcr, unsigned char active) {
    int timeout = 0;
    volatile unsigned int dummy;

    while ((DMA_CHCR(ch) & 0x01000000U) != 0) {
        if (timeout == 0x10000) {
            func_800AD698(D_8001542C, DMA_CHCR(ch));
            break;
        }
        timeout++;
    }

    if (active == 1) {
        D_800D3D40[2] |= (unsigned char)(1U << ch);
    } else {
        D_800D3D40[2] &= (unsigned char)~(1U << ch);
    }

    dummy = *(volatile unsigned int *)D_800D3D40;
    *D_800D3D3C |= (1U << ((ch << 2) + 3));

    DMA_MADR(ch) = madr;
    DMA_BCR(ch) = (ba << 16) | bs;

    while ((*D_800D3D24 & 0x40U) == 0) {
    }

    DMA_CHCR(ch) = chcr;
    dummy = DMA_CHCR(ch);
    (void)dummy;
}
