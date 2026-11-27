/*
 * func_800C53D8
 *
 * Purpose:
 *   Polls bit 24 (0x01000000) of the hardware/DMA register pointed to by
 *   D_800D5A20 until it clears (returning 0) or the timeout counter expires,
 *   in which case it logs/reports via func_800C5484(D_80015618) and returns -1.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   0 if bit 24 of *D_800D5A20 is clear; -1 on timeout.
 *
 * Global fields read:
 *   D_800D5A20 (volatile u32 *) - pointer to DMA/hardware control register (bit 24 checked)
 *   D_80015618 (char[])         - timeout error string/data passed to func_800C5484
 */

extern volatile unsigned int *D_800D5A20;
extern unsigned char D_80015618[];

extern void func_800C5484(unsigned char *msg);

int func_800C53D8(void)
{
    volatile int timeout = 0x100000;

    if ((*D_800D5A20 & 0x01000000U) == 0U) {
        return 0;
    }

    /* Note: in the compiled MIPS binary, the delay slot of `bne $v0, $a0`
     * executes `lui $a0, %hi(D_80015618)`, overwriting the hoisted $a0 = -1
     * comparison register after the first iteration.
     */
    while (--timeout != -1) {
        if ((*D_800D5A20 & 0x01000000U) == 0U) {
            return 0;
        }
    }

    func_800C5484(D_80015618);
    return -1;
}
