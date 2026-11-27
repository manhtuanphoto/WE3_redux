/*
 * func_800AAA7C
 *
 * Purpose:
 *   Steps down an audio/BGM volume level D_800E6B2A by 0x32 (50). If the
 *   signed 16-bit result drops below 0, clamps D_800E6B2A to 0 and calls
 *   func_800AAC90(). Copies the resulting volume into the left and right
 *   channel volume fields (D_800F0F30 and D_800F0F32, offsets +0x08 and +0x0A
 *   of the SPU voice struct at &D_800F0F30 - 8) and applies it via
 *   func_800C44A8().
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800E6B2A (s16/u16): current fade volume level (gp-relative)
 *   D_800F0F30 (u16):     SPU voice struct +0x08 (Left Volume)
 *   D_800F0F32 (u16):     SPU voice struct +0x0A (Right Volume)
 */

extern signed short D_800E6B2A;
extern unsigned short D_800F0F30;
extern unsigned short D_800F0F32;

extern void func_800AAC90(void);
extern void func_800C44A8(unsigned char *ctx);

void func_800AAA7C(void)
{
    signed short vol = (signed short)(D_800E6B2A - 0x32);
    unsigned short uvol;

    D_800E6B2A = vol;
    if (vol < 0) {
        D_800E6B2A = 0;
        func_800AAC90();
    }

    uvol = (unsigned short)D_800E6B2A;
    D_800F0F30 = uvol;
    D_800F0F32 = uvol;
    func_800C44A8((unsigned char *)&D_800F0F30 - 8);
}
