/*
 * func_800BA8B4 (and trailing unlabelled function sub_800BA8CC)
 *
 * Purpose:
 *   func_800BA8B4: Atomically swaps the 16-bit value at *D_800D4E7C with `val`,
 *   returning the previous 16-bit value.
 *
 *   sub_800BA8CC (unlabelled function immediately following jr $ra in func_800BA8B4.s):
 *   One-time subsystem initialization guarded by *(u16 *)D_800D3DEC == 0.
 *   Clears *D_800D4E7C and *D_800D4E78 to 0, writes 0x33333333 to *D_800D4E80,
 *   calls func_800BADD4(D_800D3DEC, 0x41A), conditionally calls func_800BA9A4(),
 *   sets D_800D3E28 = D_800D3E28 + 0xFDC, calls func_800BAE38(D_800D3DEC + 0x38),
 *   marks *(u16 *)D_800D3DEC = 1, stores callback/handle results from func_800BAE58()
 *   and func_800BAF78() into D_800D4E74+0x14 and D_800D4E74+0x04, calls func_800BAE00()
 *   and func_800ACE68(), and returns D_800D3DEC (or 0 if already initialized).
 *
 * Arguments:
 *   func_800BA8B4: val (unsigned short)
 *   sub_800BA8CC: None (void)
 *
 * Return value:
 *   func_800BA8B4: previous value of *D_800D4E7C
 *   sub_800BA8CC: pointer to D_800D3DEC on init, or 0 if already initialized
 */

#define H(p, o) (*(unsigned short *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

extern volatile unsigned short *D_800D4E78;
extern volatile unsigned short *D_800D4E7C;
extern volatile unsigned int   *D_800D4E80;
extern unsigned char           *D_800D4E74;
extern unsigned char            D_800D3DEC[];
extern unsigned int             D_800D3E28; /* Note: D_800D3E28 == D_800D3DEC + 0x3C */

extern void func_800BADD4(unsigned char *base, unsigned int size);
extern unsigned int func_800BAE48(unsigned char *ptr);
extern void func_800BA9A4(void);
extern void func_800BAE38(unsigned char *ptr);
extern unsigned int func_800BAE58(void);
extern unsigned int func_800BAF78(void);
extern void func_800BAE00(void);
extern void func_800ACE68(void);

unsigned short func_800BA8B4(unsigned short val)
{
    unsigned short prev = *D_800D4E7C;
    *D_800D4E7C = val;
    return prev;
}

/* UNCLEAR: unlabelled second function following jr $ra at 0x800BA8CC in func_800BA8B4.s */
unsigned char *sub_800BA8CC(void)
{
    if (H(D_800D3DEC, 0) != 0) {
        return 0;
    }

    *D_800D4E7C = 0;
    *D_800D4E78 = *D_800D4E7C;
    *D_800D4E80 = 0x33333333U;

    func_800BADD4(D_800D3DEC, 0x41A);
    if (func_800BAE48(D_800D3DEC + 0x38) != 0) {
        func_800BA9A4();
    }

    D_800D3E28 = (unsigned int)((unsigned char *)&D_800D3E28 + 0xFDC);
    func_800BAE38((unsigned char *)&D_800D3E28 - 4);
    H(D_800D3DEC, 0) = 1;

    W(D_800D4E74, 0x14) = func_800BAE58();
    W(D_800D4E74, 0x04) = func_800BAF78();
    func_800BAE00();
    func_800ACE68();

    return D_800D3DEC;
}
