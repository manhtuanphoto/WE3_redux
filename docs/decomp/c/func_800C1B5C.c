/*
 * func_800C1B5C (and trailing unlabelled function sub_800C1BE0)
 *
 * Purpose:
 *   func_800C1B5C: If D_800D55A8 == 0, dispatches a sequence of commands via func_800C18DC
 *   (command 2 with D_800D55A4 << D_800D55B4, command 1, and command 3 with a0/a1);
 *   otherwise calls func_800C15B8(a0, a1). Always returns `a1`.
 *
 *   sub_800C1BE0 (unlabelled function immediately following func_800C1B5C in func_800C1B5C.s):
 *   Unconditionally dispatches command 2 (D_800D55A4 << D_800D55B4), command 0, and
 *   command 3 (a0, a1) via func_800C18DC, and returns `a1`.
 *
 * Arguments:
 *   a0 (unsigned int): first parameter passed to func_800C18DC(3, a0, a1) / func_800C15B8
 *   a1 (unsigned int): second parameter and return value
 *
 * Return value:
 *   Returns `a1`.
 *
 * Global fields read:
 *   D_800D55A8 (unsigned int): mode flag
 *   D_800D55A4 (unsigned short): base value shifted by D_800D55B4
 *   D_800D55B4 (unsigned int): shift count
 */

extern unsigned int   D_800D55A8;
extern unsigned short D_800D55A4;
extern unsigned int   D_800D55B4;

extern void func_800C18DC(unsigned int cmd, unsigned int p1, unsigned int p2);
extern void func_800C15B8(unsigned int a0, unsigned int a1);

unsigned int func_800C1B5C(unsigned int a0, unsigned int a1)
{
    if (D_800D55A8 == 0) {
        /* UNCLEAR: commands 2 and 1 do not explicitly populate unused trailing args */
        func_800C18DC(2, (unsigned int)D_800D55A4 << D_800D55B4, 0);
        func_800C18DC(1, 0, 0);
        func_800C18DC(3, a0, a1);
    } else {
        func_800C15B8(a0, a1);
    }

    return a1;
}

/* UNCLEAR: unlabelled second function at 0x800C1BE0 in func_800C1B5C.s */
unsigned int sub_800C1BE0(unsigned int a0, unsigned int a1)
{
    /* UNCLEAR: commands 2 and 0 do not explicitly populate unused trailing args */
    func_800C18DC(2, (unsigned int)D_800D55A4 << D_800D55B4, 0);
    func_800C18DC(0, 0, 0);
    func_800C18DC(3, a0, a1);

    return a1;
}
