/*
 * func_800B9348
 *
 * Purpose:
 *   Configures and starts a CD-ROM / XA read operation using global state struct
 *   D_800D3C68 (PsyQ libcd CdRead-style entry point). Sets mode/retry parameters at
 *   +0x0C/+0x10 (0x200 if (mode & 0x30) == 0, 0x249 if == 0x20, else 0x246), ORs
 *   +0x0C with 0x20, stores `param0` (+0x00) and `param1` (+0x04), saves current CD
 *   callbacks (+0x24, +0x28, and +0x2C if +0x30 & 1), reads status via func_800BA518(-1),
 *   issues command 9 if (func_800B6E88() & 0xE0) != 0, and returns (func_800B90C4(0) > 0).
 *
 * Arguments:
 *   param0 (unsigned int): stored at D_800D3C68+0x00 (e.g. sector count / parameter)
 *   param1 (unsigned int): stored at D_800D3C68+0x04 (e.g. destination buffer / parameter)
 *   mode (unsigned int): stored at D_800D3C68+0x0C (transfer/CD mode flags)
 *
 * Return value:
 *   1 if func_800B90C4(0) > 0, else 0.
 *
 * Global fields read/written:
 *   D_800D3C68 (unsigned char[]): CD transfer state struct (+0x00, +0x04, +0x0C,
 *     +0x10, +0x1C, +0x24, +0x28, +0x2C, +0x30)
 */

#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned char D_800D3C68[];

extern unsigned int func_800B700C(unsigned int cb);
extern unsigned int func_800B7020(unsigned int cb);
extern unsigned int func_800B7450(unsigned int cb);
extern unsigned int func_800BA518(signed int param);
extern unsigned int func_800B6E88(void);
extern void func_800B72A4(unsigned int cmd, unsigned int a1, unsigned int a2);
extern signed int func_800B90C4(unsigned int param);

unsigned int func_800B9348(unsigned int param0, unsigned int param1, unsigned int mode)
{
    unsigned int masked;

    W(D_800D3C68, 0x0C) = mode;
    masked = W(D_800D3C68, 0x0C) & 0x30;
    if (masked == 0) {
        W(D_800D3C68, 0x10) = 0x200;
    } else if (masked == 0x20) {
        W(D_800D3C68, 0x10) = 0x249;
    } else {
        W(D_800D3C68, 0x10) = 0x246;
    }

    W(D_800D3C68, 0x0C) |= 0x20;
    W(D_800D3C68, 0x04) = param1;
    W(D_800D3C68, 0x00) = param0;

    W(D_800D3C68, 0x24) = func_800B700C(0);
    W(D_800D3C68, 0x28) = func_800B7020(0);

    if ((W(D_800D3C68, 0x30) & 1) != 0) {
        W(D_800D3C68, 0x2C) = func_800B7450(0);
    }

    W(D_800D3C68, 0x1C) = func_800BA518(-1);

    if ((func_800B6E88() & 0xE0) != 0) {
        func_800B72A4(9, 0, 0);
    }

    return (func_800B90C4(0) > 0) ? 1U : 0U;
}
