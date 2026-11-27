/*
 * func_800B2EA8
 *
 * Purpose:
 *   Stores the four PS1 GTE screen-Z FIFO data registers ($16=SZ0, $17=SZ1,
 *   $18=SZ2, $19=SZ3) as 32-bit words via `swc2` into `*s0`, `*s1`, `*s2`, `*s3`.
 *
 * Arguments:
 *   s0 (unsigned int *): destination for GTE data register $16 (SZ0)
 *   s1 (unsigned int *): destination for GTE data register $17 (SZ1)
 *   s2 (unsigned int *): destination for GTE data register $18 (SZ2)
 *   s3 (unsigned int *): destination for GTE data register $19 (SZ3)
 *
 * Return value:
 *   None (void)
 */

void func_800B2EA8(unsigned int *s0, unsigned int *s1, unsigned int *s2, unsigned int *s3)
{
    /* GTE swc2 stores 32-bit GTE data registers $16..$19 (SZ0..SZ3) directly to memory */
    __asm__ volatile (
        "swc2 $16, 0x0(%0)\n\t"
        "swc2 $17, 0x0(%1)\n\t"
        "swc2 $18, 0x0(%2)\n\t"
        "swc2 $19, 0x0(%3)"
        :
        : "r"(s0), "r"(s1), "r"(s2), "r"(s3)
        : "memory"
    );
}
