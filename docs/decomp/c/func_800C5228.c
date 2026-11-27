/*
 * func_800C5228
 *
 * Purpose:
 *   PS1 MDEC input DMA setup (analogous to PsyQ DecDCTin). Calls
 *   func_800C5344(buf, size), enables DMA channels in *D_800D5A44 (|= 0x88,
 *   likely DPCR bits for DMA0 MDECin and DMA1 MDECout), programs DMA0 base
 *   address *D_800D5A0C to buf + 1 (skipping the 4-byte MDEC command/header
 *   word), sets block control *D_800D5A10 = ((size >> 5) << 16) | 0x20, writes
 *   the first word buf[0] to *D_800D5A3C (likely MDEC0 command register), and
 *   starts DMA0 transfer via *D_800D5A14 = 0x01000201.
 *
 * Arguments:
 *   buf  (unsigned int *): MDEC input stream buffer (buf[0] = header/command word)
 *   size (unsigned int): transfer size unit (shifted >> 5 for DMA block count)
 *
 * Return value:
 *   None (void)
 *
 * Globals read / written:
 *   D_800D5A44 (volatile u32 *, R/W target): DMA primary control register pointer (DPCR)
 *   D_800D5A0C (volatile u32 *, R/W target): DMA0 MADR pointer
 *   D_800D5A10 (volatile u32 *, R/W target): DMA0 BCR pointer
 *   D_800D5A14 (volatile u32 *, R/W target): DMA0 CHCR pointer
 *   D_800D5A3C (volatile u32 *, R/W target): MDEC0 command/data register pointer
 */

extern volatile unsigned int *D_800D5A44;
extern volatile unsigned int *D_800D5A0C;
extern volatile unsigned int *D_800D5A10;
extern volatile unsigned int *D_800D5A14;
extern volatile unsigned int *D_800D5A3C;

extern void func_800C5344(unsigned int *buf, unsigned int size);

void func_800C5228(unsigned int *buf, unsigned int size)
{
    unsigned int bcr;

    func_800C5344(buf, size);

    *D_800D5A44 |= 0x88U;
    bcr = ((size >> 5) << 16) | 0x20U;

    *D_800D5A0C = (unsigned int)(buf + 1);
    *D_800D5A10 = bcr;
    *D_800D5A3C = buf[0];
    *D_800D5A14 = 0x01000201U;
}
