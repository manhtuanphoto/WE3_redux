/*
 * func_800C11D8
 *
 * Purpose:
 *   SPU subsystem initialization / reset (PsyQ _SpuInit-style routine).
 *   Calls func_800BA738(mode) and func_800C1338(mode). When mode == 0,
 *   initializes 24 halfwords ending at D_800D5176 (D_800D5148..D_800D5176) to
 *   0xC000. Then calls func_800C12C0(0xD1), clears SPU state/descriptor globals,
 *   sets D_800D5128 = D_800D55FC, calls func_800C1C44(..., D_800D55FC, 0), and
 *   zeros nine remaining SPU control words.
 *
 * Arguments:
 *   mode (int): 0 for cold init (clears 24-voice table to 0xC000); non-zero for warm/partial
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800D5176 (unsigned short[]): end of 24-entry u16 table written backwards with 0xC000
 *   D_800D55FC (unsigned int, R): default SPU attribute/address copied to D_800D5128
 *   D_800D5130 (16-byte struct, W): zeroed (+0x00 u32, +0x04 u16, +0x06 u16, +0x08 u32, +0x0C u32)
 *   D_800D5120, D_800D5124, D_800D5128, D_800D55EC, D_800D55F0, D_800D55F4,
 *   D_800D511C, D_800D55A8, D_800D5118, D_800D5144, D_800D5140, D_800D5578 (all cleared to 0)
 *
 * Notes:
 *   UNCLEAR: In the assembly, `$a0` is NOT reloaded after `func_800C12C0(0xD1)`
 *   before calling `func_800C1C44(a0, D_800D55FC, 0)`. Either `func_800C1C44`
 *   ignores its first argument or `func_800C12C0` returns/leaves a value in `$a0`.
 */

#define H(p, o) (*(unsigned short *)((unsigned char *)(p) + (o)))
#define W(p, o) (*(unsigned int *)((unsigned char *)(p) + (o)))

extern unsigned short D_800D5176[];
extern unsigned char D_800D5130[];
extern unsigned int D_800D55FC;
extern unsigned int D_800D5120;
extern unsigned int D_800D5124;
extern unsigned int D_800D5128;
extern unsigned int D_800D55EC;
extern unsigned int D_800D55F0;
extern unsigned int D_800D55F4;
extern unsigned int D_800D511C;
extern unsigned int D_800D55A8;
extern unsigned int D_800D5118;
extern unsigned int D_800D5144;
extern unsigned int D_800D5140;
extern unsigned int D_800D5578;

extern void func_800BA738(int mode);
extern void func_800C1338(int mode);
extern unsigned int func_800C12C0(unsigned int arg);
extern void func_800C1C44(unsigned int a0_unclear, unsigned int a1, unsigned int a2);

void func_800C11D8(int mode)
{
    int i;
    unsigned short *p;

    func_800BA738(mode);
    func_800C1338(mode);

    if (mode == 0) {
        p = D_800D5176;
        for (i = 23; i >= 0; i--) {
            *p-- = 0xC000u;
        }
    }

    func_800C12C0(0xD1u);

    D_800D5120 = 0;
    D_800D5124 = 0;
    W(D_800D5130, 0x00) = 0;
    H(D_800D5130, 0x04) = 0;
    H(D_800D5130, 0x06) = 0;
    W(D_800D5130, 0x08) = 0;
    W(D_800D5130, 0x0C) = 0;
    D_800D5128 = D_800D55FC;

    /* UNCLEAR: $a0 is not reloaded after func_800C12C0(0xD1) before this call */
    func_800C1C44(0xD1u, D_800D55FC, 0u);

    D_800D55EC = 0;
    D_800D55F0 = 0;
    D_800D55F4 = 0;
    D_800D511C = 0;
    D_800D55A8 = 0;
    D_800D5118 = 0;
    D_800D5144 = 0;
    D_800D5140 = 0;
    D_800D5578 = 0;
}
