/*
 * func_800B8760
 *
 * Purpose:
 *   PsyQ low-level CD-ROM subsystem initialization. Logs banner strings,
 *   clears callback/status globals, installs interrupt callback D_800B8CA0,
 *   drains pending CD status FIFO, sets hardware mode registers (0x1325), and
 *   issues initial CD commands (1, 0x0A, 0x0C) via func_800B80BC.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   0 on success; -1 if commands 0x0A/0x0C fail or func_800B7B74(0, 0) != 2.
 *
 * Global fields read/written:
 *   D_800153B8 / D_800153C4 / D_800D3C30 (strings)
 *   D_800D396D, D_800D396C, D_800D3950, D_800D394C, D_800D3960, D_800D395C (cleared; +0x10 bit tested)
 *   D_800D3C14, D_800D3C1C, D_800D3C20, D_800D3C24 (volatile hardware register pointers)
 *   D_800D3C2C (u8[3]) completion status bytes
 *   D_800B8CA0 (interrupt callback)
 */

extern unsigned char D_800153B8[];
extern unsigned char D_800153C4[];
extern unsigned char D_800D3C30[];
extern unsigned char D_800D396D;
extern unsigned char D_800D396C;
extern unsigned int  D_800D3950;
extern unsigned int  D_800D394C;
extern unsigned int  D_800D3960;
extern unsigned int  D_800D395C;
extern volatile unsigned char *D_800D3C14;
extern volatile unsigned char *D_800D3C1C;
extern volatile unsigned char *D_800D3C20;
extern volatile unsigned int  *D_800D3C24;
extern unsigned char D_800D3C2C[];
extern unsigned char D_800B8CA0[];

extern void func_800B8D80(unsigned char *str);
extern void func_800AD698(unsigned char *fmt, unsigned char *arg);
extern void func_800BA738(void);
extern void func_800BA768(int spec, void *cb);
extern int  func_800B80BC(unsigned int chan, unsigned char *cmdBuf, unsigned char *outBuf, int asyncFlag);
extern int  func_800B7B74(int a0, int a1);

signed int func_800B8760(void)
{
    func_800B8D80(D_800153B8);
    func_800AD698(D_800153C4, D_800D3C30);

    D_800D396D = 0;
    D_800D396C = 0;
    D_800D3950 = 0;
    D_800D394C = 0;
    D_800D3960 = 0;
    D_800D395C = 0;

    func_800BA738();
    func_800BA768(2, D_800B8CA0);

    *D_800D3C14 = 1;
    while ((*D_800D3C20 & 7U) != 0) {
        *D_800D3C14 = 1;
        *D_800D3C20 = 7;
        *D_800D3C1C = 7;
    }

    D_800D3C2C[2] = 0;
    D_800D3C2C[1] = 0;
    D_800D3C2C[0] = 2;
    *D_800D3C14 = 0;
    *D_800D3C20 = 0;
    *D_800D3C24 = 0x1325U;

    func_800B80BC(1, 0, 0, 0);
    if ((D_800D395C & 0x10U) != 0) {
        func_800B80BC(1, 0, 0, 0);
    }

    if (func_800B80BC(0xA, 0, 0, 0) != 0) {
        return -1;
    }
    if (func_800B80BC(0xC, 0, 0, 0) != 0) {
        return -1;
    }
    if (func_800B7B74(0, 0) != 2) {
        return -1;
    }

    return 0;
}
