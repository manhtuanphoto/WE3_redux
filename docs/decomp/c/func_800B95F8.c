/*
 * func_800B95F8 - Reset subsystem state/counters and adjacent CD/SPU reset entry
 *
 * Purpose:
 *   Part 1 (`func_800B95F8`, lines 1-24): Clears state globals D_8010C1CC,
 *   D_8010C1C4, D_8010C1C0, D_800FF680, calls func_800B9908(0, D_8010CE68),
 *   and clears D_800EF91C, D_800E81CC, D_800E7654.
 *   Part 2 (adjacent unlabeled function at +0x60, lines 25-52): Calls
 *   func_800AD3D8(), checks D_800D3970 == 1 to select between
 *   (func_800C4E54, func_800C4E2C) and (func_800B7450, func_800B7020), clears
 *   *D_800D3CB4 and *D_800D3CC0, and calls func_800ACE68.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Globals read & written:
 *   D_8010CE68 (u32, R): passed to func_800B9908
 *   D_8010C1CC, D_8010C1C4, D_8010C1C0, D_800FF680 (u32, W): cleared to 0
 *   D_800EF91C, D_800E7654 (u32, W): cleared to 0
 *   D_800E81CC (u16, W): cleared to 0
 *   D_800D3970 (u32, R): mode selector (1 vs other)
 *   D_800D3CB4, D_800D3CC0 (u8 *, R): byte pointers cleared to 0
 */

extern unsigned int D_8010CE68;
extern unsigned int D_8010C1CC;
extern unsigned int D_8010C1C4;
extern unsigned int D_8010C1C0;
extern unsigned int D_800FF680;
extern unsigned int D_800EF91C;
extern unsigned short D_800E81CC;
extern unsigned int D_800E7654;

extern unsigned int D_800D3970;
extern unsigned char *D_800D3CB4;
extern unsigned char *D_800D3CC0;

extern void func_800B9908(int a0, unsigned int a1);
extern void func_800AD3D8(void);
extern void func_800C4E54(int a0);
extern void func_800C4E2C(int a0);
extern void func_800B7450(int a0);
extern void func_800B7020(int a0);
extern void func_800ACE68(void);

void func_800B95F8(void)
{
    D_8010C1CC = 0;
    D_8010C1C4 = 0;
    D_8010C1C0 = 0;
    D_800FF680 = 0;

    func_800B9908(0, D_8010CE68);

    D_800EF91C = 0;
    D_800E81CC = 0;
    D_800E7654 = 0;
}

/* UNCLEAR: unlabeled second function immediately following func_800B95F8 in .s (at +0x60) */
void func_800B9658(void)
{
    func_800AD3D8();

    if (D_800D3970 == 1u) {
        func_800C4E54(0);
        func_800C4E2C(0);
    } else {
        func_800B7450(0);
        func_800B7020(0);
    }

    *D_800D3CB4 = 0;
    *D_800D3CC0 = 0;
    /* Note: func_800ACE68 takes no initialized $a0 here; `sb $zero, 0($v0)` is in its delay slot */
    func_800ACE68();
}
