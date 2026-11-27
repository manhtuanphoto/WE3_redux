/*
 * func_800A995C
 *
 * Purpose:
 *   Initializes SPU/audio buffers, common SPU master volume attributes
 *   (D_800EF870 via func_800C3FC8), and two SPU voice attribute structures
 *   (at D_800F0F2C - 4 and D_800F0EEC - 4, submitted via func_800C44A8),
 *   using volume scaled from D_800CE750 << 7 for the second voice. Clears
 *   countdown globals D_800E6B84 and D_800E6AD4.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800CE750 (u16): volume/gain factor shifted << 7 for second SPU voice
 *
 * Global fields written:
 *   D_800E6B70 (u32),
 *   D_800EF870 / D_800EF874 / D_800EF876 (SPU common attr),
 *   D_800F0F2C (-0x04..+0x34) first SPU voice attr block,
 *   D_800F0EEC (-0x04..+0x34) second SPU voice attr block,
 *   D_800E6B84 (s16) = 0, D_800E6AD4 (s16) = 0
 */

#define H(p, o) (*(unsigned short *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned int   D_800E6B70;
extern unsigned short D_800E6B84;
extern unsigned short D_800E6AD4;
extern unsigned short D_800CE750;
extern unsigned int   D_800EF870;
extern unsigned short D_800EF874;
extern unsigned short D_800EF876;
extern unsigned char  D_800F0F2C[];
extern unsigned char  D_800F0EEC[];

extern unsigned int func_800C2838(unsigned int a0, unsigned int a1);
extern void func_800C3DB8(unsigned int a0);
extern void func_800C3FC8(unsigned int *attr);
extern void func_800C44A8(unsigned char *voiceAttr);

void func_800A995C(void)
{
    unsigned short scaledVol;

    D_800E6B70 = func_800C2838(0x1010, 0x20000);
    D_800E6B70 = func_800C2838(0x21010, 0x20000);
    func_800C3DB8(0x1010);

    D_800EF870 = 3;
    D_800EF874 = 0x7FFF;
    D_800EF876 = 0x7FFF;
    func_800C3FC8(&D_800EF870);

    /* Configure first SPU voice/channel attribute block (D_800F0F2C - 4) */
    W(D_800F0F2C, -0x04) = 0x00400000U;
    W(D_800F0F2C,  0x00) = 0xFF93U;
    H(D_800F0F2C,  0x04) = 0x3FFFU;
    H(D_800F0F2C,  0x06) = 0x3FFFU;
    H(D_800F0F2C,  0x10) = 0x05CEU;
    W(D_800F0F2C,  0x18) = 0x1010U;
    W(D_800F0F2C,  0x20) = 1U;
    W(D_800F0F2C,  0x24) = 1U;
    W(D_800F0F2C,  0x28) = 3U;
    H(D_800F0F2C,  0x2C) = 0U;
    H(D_800F0F2C,  0x2E) = 0U;
    H(D_800F0F2C,  0x30) = 0U;
    H(D_800F0F2C,  0x32) = 0U;
    H(D_800F0F2C,  0x34) = 0xFU;
    W(D_800F0F2C,  0x1C) = 0x1050U;
    func_800C44A8(D_800F0F2C - 0x04);

    /* Configure second SPU voice/channel attribute block (D_800F0EEC - 4) */
    W(D_800F0EEC, -0x04) = 0x00800000U;
    W(D_800F0EEC,  0x00) = 0xFF93U;
    scaledVol = (unsigned short)(D_800CE750 << 7);
    H(D_800F0EEC,  0x10) = 0x0A20U;
    W(D_800F0EEC,  0x18) = 0x21010U;
    W(D_800F0EEC,  0x20) = 1U;
    W(D_800F0EEC,  0x24) = 1U;
    W(D_800F0EEC,  0x28) = 3U;
    H(D_800F0EEC,  0x2C) = 0U;
    H(D_800F0EEC,  0x2E) = 0U;
    H(D_800F0EEC,  0x30) = 0U;
    H(D_800F0EEC,  0x32) = 0U;
    H(D_800F0EEC,  0x34) = 0xFU;
    W(D_800F0EEC,  0x1C) = 0x21050U;
    H(D_800F0EEC,  0x04) = scaledVol;
    H(D_800F0EEC,  0x06) = scaledVol;
    func_800C44A8(D_800F0EEC - 0x04);

    D_800E6B84 = 0;
    D_800E6AD4 = 0;
}
