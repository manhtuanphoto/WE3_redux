/*
 * func_800C4D98
 *
 * Purpose:
 *   Reads raw left and right hardware volume registers for SPU voice `ch`
 *   (16-byte stride from base pointer D_800D558C), converts 15-bit signed values
 *   in [0x4000..0xFFFF] by subtracting 0x8000 (sign-extending 15-bit negative
 *   SPU volumes into 16-bit signed shorts), and stores them into `*out_left`
 *   and `*out_right` (PsyQ SpuGetVoiceVolume-style helper).
 *
 * Arguments:
 *   ch        (signed int):     SPU voice channel index
 *   out_left  (signed short *): destination for left channel volume
 *   out_right (signed short *): destination for right channel volume
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800D558C (unsigned char *): base pointer to SPU voice hardware registers
 *                                 (16 bytes per voice: +0x00 volL, +0x02 volR)
 */

extern unsigned char *D_800D558C;

void func_800C4D98(signed int ch, signed short *out_left, signed short *out_right)
{
    volatile unsigned short *voice_regs =
        (volatile unsigned short *)(D_800D558C + (ch << 4));
    unsigned int vol_l = voice_regs[0];
    unsigned int vol_r = voice_regs[1];

    if (vol_l >= 0x4000U) {
        *out_left = (signed short)(vol_l - 0x8000U);
    } else {
        *out_left = (signed short)vol_l;
    }

    if (vol_r >= 0x4000U) {
        *out_right = (signed short)(vol_r - 0x8000U);
    } else {
        *out_right = (signed short)vol_r;
    }
}
