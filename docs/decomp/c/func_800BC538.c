/*
 * func_800BC538
 *
 * Purpose:
 *   Initializes the PS1 SPU hardware registers and sound driver channel table.
 *   Copies 8 default halfwords from D_800D4F18 to all 24 SPU voices (0x1F801C00..0x1F801D7E),
 *   copies 16 default halfwords from D_800D4F28 to SPU control registers (0x1F801D80..0x1F801D9E),
 *   calls func_800BE938(24), zeroes 32 64-byte structs in D_8010C540, sets D_8010C1BC = 0x3C,
 *   and clears D_800FF72C and D_800F1014.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800D4F18 (unsigned short[8]): default SPU per-voice register values
 *   D_800D4F28 (unsigned short[16]): default SPU control register values
 *
 * Global fields written:
 *   D_8010C540 (32 x 64-byte structs, zeroed)
 *   D_8010C1BC (u32) = 0x3C
 *   D_800FF72C (u32) = 0
 *   D_800F1014 (u32) = 0
 */

#define SPU_VOICE_BASE ((volatile unsigned short *)0x1F801C00)
#define SPU_CTRL_BASE  ((volatile unsigned short *)0x1F801D80)

extern const unsigned short D_800D4F18[8];
extern const unsigned short D_800D4F28[16];
extern unsigned int D_8010C540[32][16];
extern unsigned int D_8010C1BC;
extern unsigned int D_800FF72C;
extern unsigned int D_800F1014;

extern void func_800BE938(unsigned int num_voices);

void func_800BC538(void)
{
    volatile unsigned short *dst;
    int i;
    int j;

    dst = SPU_VOICE_BASE;
    for (i = 0; i < 24; i++) {
        for (j = 0; j < 8; j++) {
            *dst++ = D_800D4F18[j];
        }
    }

    dst = SPU_CTRL_BASE;
    for (i = 0; i < 16; i++) {
        *dst++ = D_800D4F28[i];
    }

    func_800BE938(24);

    for (i = 0; i < 32; i++) {
        for (j = 15; j >= 0; j--) {
            D_8010C540[i][j] = 0;
        }
    }

    D_8010C1BC = 0x3C;
    D_800FF72C = 0;
    D_800F1014 = 0;
}
