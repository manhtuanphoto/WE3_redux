/*
 * func_800AD7F8
 *
 * Purpose:
 *   Swaps `size` bytes in place between two buffers `bufA` and `bufB`.
 *
 * Arguments:
 *   bufA (unsigned char *): First buffer
 *   bufB (unsigned char *): Second buffer
 *   size (unsigned int): Number of bytes to swap
 *
 * Return value:
 *   None (void)
 */

void func_800AD7F8(unsigned char *bufA, unsigned char *bufB, unsigned int size)
{
    unsigned int i;

    if (size == 0) {
        return;
    }

    for (i = 0; i < size; i++) {
        unsigned char a = bufA[i];
        unsigned char b = bufB[i];
        bufA[i] = b;
        bufB[i] = a;
    }
}
