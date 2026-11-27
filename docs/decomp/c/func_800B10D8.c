/*
 * func_800B10D8
 *
 * Purpose:
 *   Fills `len` consecutive bytes starting at `dst` with `(unsigned char)val`
 *   (`memset`-style byte fill without returning `dst`). Does nothing if
 *   `len == 0`.
 *
 * Arguments:
 *   dst (unsigned char *): destination byte buffer
 *   val (int): byte fill value (low 8 bits written)
 *   len (unsigned int): number of bytes to fill
 *
 * Return value:
 *   None (void)
 */

void func_800B10D8(unsigned char *dst, int val, unsigned int len)
{
    while (len != 0U) {
        *dst++ = (unsigned char)val;
        len--;
    }
}
