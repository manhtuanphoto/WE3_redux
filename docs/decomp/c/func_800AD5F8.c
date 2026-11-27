/*
 * func_800AD5F8
 *
 * Purpose:
 *   PS1 BIOS A(0x15) trampoline (strcat).
 *
 *   Note: The raw assembly file also contains 4 immediately following unlabelled
 *   PS1 BIOS A0-table trampolines without intermediate returns:
 *     +0x00 (func_800AD5F8): A(0x15) - strcat(dst, src)
 *     +0x0C (0x800AD604):    A(0x18) - strncmp(s1, s2, n)
 *     +0x18 (0x800AD610):    A(0x19) - strcpy(dst, src)
 *     +0x24 (0x800AD61C):    A(0x1A) - strncpy(dst, src, n)
 *     +0x30 (0x800AD628):    A(0x1C) - strchr(s, c)
 *
 * Arguments:
 *   dst (char *): Destination NUL-terminated string
 *   src (const char *): Source NUL-terminated string
 *
 * Return value:
 *   char *: Pointer to dst (returned by BIOS A(0x15))
 */

typedef char *(*bios_a0_strcat_fn)(char *dst, const char *src);

char *func_800AD5F8(char *dst, const char *src)
{
    /* BIOS A0 vector at 0x000000A0 with $t1 = 0x15 (strcat) */
    register int fn_id __asm__("$t1") = 0x15;
    bios_a0_strcat_fn bios_a0 = (bios_a0_strcat_fn)0x000000A0;

    __asm__ volatile ("" : : "r"(fn_id));
    return bios_a0(dst, src);
}
