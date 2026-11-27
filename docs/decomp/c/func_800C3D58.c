/*
 * func_800C3D58
 *
 * Purpose:
 *   SPU RAM transfer write helper (PsyQ SpuWrite-style).
 *   Clamps `size` to the maximum transfer size 0x7EFF0, initiates the transfer
 *   via func_800C1B5C(buf, size), clears D_800D55C0 if D_800D55C4 == 0, and
 *   returns the clamped transfer size.
 *
 * Arguments:
 *   buf  (unsigned char *): source buffer in main RAM
 *   size (unsigned int):    requested byte count to transfer
 *
 * Return value:
 *   unsigned int: clamped byte count transferred (<= 0x7EFF0)
 *
 * Global fields read / written:
 *   D_800D55C4 (unsigned int, R):  transfer mode/flag
 *   D_800D55C0 (unsigned int, W):  cleared to 0 when D_800D55C4 == 0
 */

extern unsigned int D_800D55C4;
extern unsigned int D_800D55C0;

extern unsigned int func_800C1B5C(unsigned char *buf, unsigned int size);

unsigned int func_800C3D58(unsigned char *buf, unsigned int size)
{
    if (size > 0x7EFF0U) {
        size = 0x7EFF0U;
    }

    func_800C1B5C(buf, size);

    if (D_800D55C4 == 0U) {
        D_800D55C0 = 0U;
    }

    return size;
}
