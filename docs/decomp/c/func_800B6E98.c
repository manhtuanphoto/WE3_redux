/*
 * func_800B6E98 (and trailing unlabelled function sub_800B6EA8)
 *
 * Purpose:
 *   func_800B6E98: Returns the 8-bit unsigned global D_800D396C (CD subsystem state byte).
 *
 *   sub_800B6EA8 (unlabelled function immediately following `jr $ra` in
 *   func_800B6E98.s): Returns the 8-bit unsigned global D_800D396D (current CD
 *   command/channel index byte).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   func_800B6E98: unsigned char - value of D_800D396C
 *   sub_800B6EA8:  unsigned char - value of D_800D396D
 *
 * Global fields read:
 *   D_800D396C (unsigned char)
 *   D_800D396D (unsigned char)
 */

extern unsigned char D_800D396C;
extern unsigned char D_800D396D;

unsigned char func_800B6E98(void)
{
    return D_800D396C;
}

/* UNCLEAR: unlabelled second function following jr $ra at 0x800B6EA8 in func_800B6E98.s */
unsigned char sub_800B6EA8(void)
{
    return D_800D396D;
}
