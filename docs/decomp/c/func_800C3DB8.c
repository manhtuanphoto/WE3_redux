/*
 * func_800C3DB8
 *
 * Purpose:
 *   Sets the SPU RAM transfer start address (PsyQ SpuSetTransferStartAddr-style).
 *   Checks that `spu_addr` lies in the valid user SPU RAM range [0x1010, 0x7FFF8]
 *   (i.e. (spu_addr - 0x1010) <= 0x7EFE8). If valid, aligns/shifts the address
 *   via func_800C1C88(-1, spu_addr), caches the 16-bit SPU-unit address in
 *   D_800D55A4, and returns the aligned byte address (D_800D55A4 << D_800D55B4).
 *   Returns 0 if `spu_addr` is out of range.
 *
 * Arguments:
 *   spu_addr (unsigned int): target address in SPU RAM
 *
 * Return value:
 *   unsigned int: aligned SPU RAM byte address, or 0 if out of bounds
 *
 * Global fields read / written:
 *   D_800D55A4 (unsigned short, W/R): aligned SPU-unit transfer start address
 *   D_800D55B4 (unsigned int, R):     bit shift converting SPU units to bytes
 */

extern unsigned short D_800D55A4;
extern unsigned int   D_800D55B4;

extern unsigned int func_800C1C88(int slot, unsigned int val);

unsigned int func_800C3DB8(unsigned int spu_addr)
{
    if ((spu_addr - 0x1010U) > 0x7EFE8U) {
        return 0U;
    }

    D_800D55A4 = (unsigned short)func_800C1C88(-1, spu_addr);
    return (unsigned int)D_800D55A4 << D_800D55B4;
}
