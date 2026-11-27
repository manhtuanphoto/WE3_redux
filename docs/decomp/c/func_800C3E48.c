/*
 * func_800C3E48
 *
 * Purpose:
 *   PsyQ SpuWrite equivalent. Clamps transfer byte `size` to the maximum usable
 *   SPU RAM size (0x7EFF0 bytes), transfers `size` bytes from `addr` via
 *   func_800C1B5C(addr, size), advances the SPU transfer address via
 *   func_800C1C88(-1, (D_800D55A4 << D_800D55B4) + size), stores the updated
 *   shifted halfword address back into D_800D55A4, clears D_800D55C0 when
 *   D_800D55C4 == 0, and returns the clamped byte count.
 *
 * Arguments:
 *   addr (void *): source buffer in main RAM
 *   size (unsigned int): requested transfer size in bytes
 *
 * Return value:
 *   unsigned int: actual clamped transfer size in bytes (<= 0x7EFF0)
 *
 * Globals read / written:
 *   D_800D55A4 (u16, R/W): current SPU RAM address (shifted right by D_800D55B4)
 *   D_800D55B4 (u32, R): SPU address bit shift
 *   D_800D55C4 (u32, R): transfer mode flag (0 = clear D_800D55C0 after call)
 *   D_800D55C0 (u32, W): transfer status/wait flag
 */

extern unsigned short D_800D55A4;
extern unsigned int D_800D55B4;
extern unsigned int D_800D55C4;
extern unsigned int D_800D55C0;

extern void func_800C1B5C(void *addr, unsigned int size);
extern unsigned short func_800C1C88(signed int arg0, unsigned int spu_addr);

unsigned int func_800C3E48(void *addr, unsigned int size)
{
    unsigned int spu_addr;

    if (size > 0x7EFF0U) {
        size = 0x7EFF0U;
    }

    spu_addr = (unsigned int)D_800D55A4 << D_800D55B4;
    func_800C1B5C(addr, size);
    D_800D55A4 = func_800C1C88(-1, spu_addr + size);

    if (D_800D55C4 == 0) {
        D_800D55C0 = 0;
    }

    return size;
}
