/*
 * func_800B90C4
 *
 * Purpose:
 *   PS1 libcd initialization / reset helper (sets sync & ready callbacks, checks
 *   CD shell-open status, probes CD mode/status, installs ready callback
 *   func_800B8D80+0x18, and initializes D_800D3C68 state block).
 *   Also includes an unlabelled trailing function in the same asm_compact unit
 *   (starts at line 120 after func_800B90C4's jr $ra) that clears callbacks and
 *   issues CD command 9.
 *
 * Arguments (func_800B90C4):
 *   arg0 (int): when nonzero, sends CD mode/init control commands (cmd 9 and cmd E)
 *
 * Return value:
 *   VSync(-1) timestamp or -1 on error/failure (stored in D_800D3C68 + 0x14).
 *
 * Global fields read/written:
 *   D_800D3C68..D_800D3C98: libcd state block (D_800D3C74 = D_800D3C68 + 0x0C,
 *     D_800D3C98 = D_800D3C68 + 0x30; bit 0 = debug print enable)
 *   D_800153F4, D_8001540C, D_800B8FF8: debug/error strings passed to func_800B8D80 / func_800B7450
 *   func_800B8D80 + 0x18: callback address passed to func_800B7020
 */

#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char D_800D3C68[];
extern unsigned int  D_800D3C74;
extern unsigned int  D_800D3C98;
extern char D_800153F4[];
extern char D_8001540C[];
extern char D_800B8FF8[];
extern unsigned char func_800B8D80[]; /* +0x18 used as callback pointer */

extern void func_800B700C(unsigned int cb);
extern void func_800B7020(unsigned int cb);
extern void func_800B7450(unsigned int a0);
extern void func_800B7474(unsigned int a0);
extern unsigned int func_800B6E88(void);
extern unsigned int func_800B6E98(void);
extern unsigned int func_800B6EB8(void);
extern unsigned int func_800B6F30(void);
extern unsigned int func_800BA518(int arg);
extern void func_800B8D80(const char *msg);
extern int func_800B7170(int cmd, unsigned char *buf);
extern int func_800B7034(int cmd, unsigned int arg1, unsigned int arg2);
extern unsigned int func_800B7598(unsigned int mode);

int func_800B90C4(int arg0)
{
    unsigned char sp10;

    func_800B700C(0);
    func_800B7020(0);

    if ((D_800D3C98 & 1) != 0) {
        func_800B7450(0);
    }

    if ((func_800B6E88() & 0x10) != 0) {
        if ((func_800BA518(-1) & 0x3F) == 0) {
            func_800B8D80(D_800153F4);
        }
        func_800B7170(1, 0);
        W(D_800D3C68, 0x1C) = func_800BA518(-1);
        SW(D_800D3C68, 0x14) = -1;
        return SW(D_800D3C68, 0x14);
    }

    if (arg0 != 0) {
        func_800B8D80(D_8001540C);
        func_800B7034(9, 0, 0);
        if (func_800B7034(2, func_800B6EB8(), 0) == 0) {
            SW(D_800D3C68, 0x14) = -1;
            return SW(D_800D3C68, 0x14);
        }
    }

    func_800B6F30();
    sp10 = (unsigned char)D_800D3C74;
    if ((D_800D3C74 & 0xFF) != func_800B6E98() || arg0 != 0) {
        if (func_800B7034(0xE, (unsigned int)&sp10, 0) == 0) {
            SW(D_800D3C68, 0x14) = -1;
            return SW(D_800D3C68, 0x14);
        }
    }

    W(D_800D3C68, 0x20) = func_800B7598(func_800B6EB8());
    /* UNCLEAR: sets ready callback to internal address func_800B8D80 + 0x18 */
    func_800B7020((unsigned int)(func_800B8D80 + 0x18));

    if ((W(D_800D3C68, 0x30) & 1) != 0) {
        func_800B7450((unsigned int)D_800B8FF8);
    }

    W(D_800D3C68, 0x08) = W(D_800D3C68, 0x04);
    func_800B7170(6, 0);

    {
        unsigned int ts = func_800BA518(-1);
        SW(D_800D3C68, 0x14) = (signed int)ts;
        W(D_800D3C68, 0x18) = ts;
    }

    return SW(D_800D3C68, 0x14);
}

/*
 * UNCLEAR: Unlabelled trailing function in func_800B90C4.s (lines 120-153,
 * immediately following func_800B90C4's jr $ra; likely CdDeinit / CdReset helper).
 */
int func_800B90C4_tail(void)
{
    if ((D_800D3C98 & 1) != 0) {
        func_800B7474(0);
    }

    W(D_800D3C68, 0x14) = 0;
    func_800B700C(W(D_800D3C68, 0x24));
    func_800B7020(W(D_800D3C68, 0x28));

    if ((D_800D3C98 & 1) != 0) {
        func_800B7450(W(D_800D3C68, 0x2C));
    }

    return func_800B7170(9, 0);
}
