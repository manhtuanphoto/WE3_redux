/*
 * func_800B8714
 *
 * Purpose:
 *   Resets CD-ROM callback/status globals (D_800D3950, D_800D394C, D_800D3960,
 *   D_800D395C), calls func_800BA738(), and registers the CD-ROM interrupt
 *   callback D_800B8CA0 on channel 2 via func_800BA768(2, D_800B8CA0).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields written:
 *   D_800D3950, D_800D394C, D_800D3960, D_800D395C (u32): cleared to 0
 *
 * Global symbols referenced:
 *   D_800B8CA0: CD-ROM interrupt callback passed to func_800BA768
 */

extern unsigned int D_800D3950;
extern unsigned int D_800D394C;
extern unsigned int D_800D3960;
extern unsigned int D_800D395C;
extern unsigned char D_800B8CA0[];

extern void func_800BA738(void);
extern void func_800BA768(int spec, void *cb);

void func_800B8714(void)
{
    D_800D3950 = 0;
    D_800D394C = 0;
    D_800D3960 = 0;
    D_800D395C = 0;

    func_800BA738();
    func_800BA768(2, D_800B8CA0);
}
