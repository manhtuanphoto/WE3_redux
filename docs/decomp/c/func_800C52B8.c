/*
 * func_800C52B8
 *
 * Purpose:
 *   PS1 MDEC output DMA setup (analogous to PsyQ DecDCTout). Calls
 *   func_800C53D8(buf, size), enables DMA channels in *D_800D5A44 (|= 0x88,
 *   likely DPCR bits for DMA0 MDECin and DMA1 MDECout), clears *D_800D5A20
 *   (likely DMA1 CHCR), sets DMA1 base address *D_800D5A18 = buf, sets block
 *   control *D_800D5A1C = ((size >> 5) << 16) | 0x20, and starts DMA1 transfer
 *   via *D_800D5A20 = 0x01000200 (MDEC to RAM).
 *
 * Arguments:
 *   buf  (void *): destination buffer in main RAM for decoded MDEC output
 *   size (unsigned int): transfer size unit (shifted >> 5 for DMA block count)
 *
 * Return value:
 *   None (void)
 *
 * Globals read / written:
 *   D_800D5A44 (volatile u32 *, R/W target): DMA primary control register pointer (DPCR)
 *   D_800D5A18 (volatile u32 *, W target): DMA1 MADR pointer
 *   D_800D5A1C (volatile u32 *, W target): DMA1 BCR pointer
 *   D_800D5A20 (volatile u32 *, W target): DMA1 CHCR pointer
 */

extern volatile unsigned int *D_800D5A44;
extern volatile unsigned int *D_800D5A18;
extern volatile unsigned int *D_800D5A1C;
extern volatile unsigned int *D_800D5A20;

extern void func_800C53D8(void *buf, unsigned int size);

void func_800C52B8(void *buf, unsigned int size)
{
    unsigned int bcr;

    func_800C53D8(buf, size);

    *D_800D5A44 |= 0x88U;
    *D_800D5A20 = 0U;

    bcr = ((size >> 5) << 16) | 0x20U;
    *D_800D5A18 = (unsigned int)buf;
    *D_800D5A1C = bcr;
    *D_800D5A20 = 0x01000200U;
}
