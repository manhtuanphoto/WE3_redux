/*
 * func_800AD648
 *
 * Purpose:
 *   PS1 BIOS A0 vector trampoline: A(27h) - strrchr(s, c).
 *   Loads $t2 = 0xA0, $t1 = 0x27 and tail-jumps to 0x000000A0, forwarding
 *   $a0, $a1, and $ra.
 *
 * Arguments:
 *   s (const char *): Null-terminated string to search ($a0)
 *   c (int): Character to locate ($a1)
 *
 * Return value:
 *   char *: Pointer to last occurrence of c in s, or NULL (returned by BIOS in $v0)
 */

char *func_800AD648(const char *s, int c)
{
    /* BIOS call: $t2 = 0xA0, $t1 = 0x27 (strrchr), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x27 and tail-jump to 0x000000A0 without inline asm. */
    const char *last = (const char *)0;
    char ch = (char)c;

    while (*s != '\0') {
        if (*s == ch) {
            last = s;
        }
        s++;
    }
    if (ch == '\0') {
        return (char *)s;
    }
    return (char *)last;
}
