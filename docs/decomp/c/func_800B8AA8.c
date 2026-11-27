/*
 * func_800B8AA8
 *
 * Purpose:
 *   Low-level CD-ROM / DMA channel 3 hardware transfer routine using indirect MMIO
 *   register pointers (D_800D3C14..D_800D3C58). Configures bus/DMA timing and control
 *   registers, sets destination address (a0) and block/transfer count (a1 | 0x10000),
 *   polls ready bit 0x40, starts DMA (0x11000000), and waits for completion (bit 24).
 *
 * Arguments:
 *   madr (unsigned int): DMA memory address written to *D_800D3C50
 *   bcr (unsigned int): DMA block control value (ORed with 0x10000) written to *D_800D3C54
 *
 * Return value:
 *   0
 *
 * Global fields read:
 *   D_800D3C14, D_800D3C20, D_800D3C24, D_800D3C48,
 *   D_800D3C4C, D_800D3C50, D_800D3C54, D_800D3C58 (indirect hardware register pointers)
 */

/* UNCLEAR: exact MMIO register addresses held by D_800D3Cxx pointers */
extern volatile unsigned char *D_800D3C14;
extern volatile unsigned char *D_800D3C20;
extern volatile unsigned int  *D_800D3C24;
extern volatile unsigned int  *D_800D3C48;
extern volatile unsigned int  *D_800D3C4C;
extern volatile unsigned int  *D_800D3C50;
extern volatile unsigned int  *D_800D3C54;
extern volatile unsigned int  *D_800D3C58;

unsigned int func_800B8AA8(unsigned int madr, unsigned int bcr)
{
    *D_800D3C14 = 0;
    *D_800D3C20 = 0x80;
    *D_800D3C48 = 0x20943;
    *D_800D3C24 = 0x1323;
    *D_800D3C4C |= 0x8000;
    *D_800D3C50 = madr;
    *D_800D3C54 = bcr | 0x10000;

    while ((*D_800D3C14 & 0x40) == 0) {
    }

    *D_800D3C58 = 0x11000000;
    while ((*D_800D3C58 & 0x01000000) != 0) {
    }

    *D_800D3C24 = 0x1325;
    return 0;
}
