/*
 * func_800AAA10
 *
 * Purpose:
 *   Steps down SPU voice 22 fade-out volume (D_800E6B2A) by 0x320 (800) per call,
 *   clamping at 0, writes the updated left/right volumes into the SpuVoiceAttr-like
 *   buffer at D_800F0F2C - 4 (mask = SPU_VOICE_VOLL | SPU_VOICE_VOLR, voice = 1 << 22),
 *   and submits it via func_800C44A8 (SpuSetVoiceAttr).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read / written:
 *   D_800E6B2A (unsigned short, gp-relative): current fade volume (stepped down by 0x320)
 *   D_800F0F2C (unsigned char[], struct base at -4):
 *     -0x04 (u32): voice bitmask (0x00400000 = voice 22)
 *     +0x00 (u32): attribute mask (0x00000003 = left + right volume)
 *   D_800F0F30 (unsigned short): left channel volume (+0x04 from D_800F0F2C)
 *   D_800F0F32 (unsigned short): right channel volume (+0x06 from D_800F0F2C)
 */

#define W(p, o) (*(unsigned int *)((unsigned char *)(p) + (o)))

extern unsigned short D_800E6B2A;
extern unsigned char  D_800F0F2C[];
extern unsigned short D_800F0F30;
extern unsigned short D_800F0F32;

extern void func_800C44A8(unsigned char *attr);

void func_800AAA10(void)
{
    signed short vol;

    W(D_800F0F2C, 0x00) = 0x3U;
    W(D_800F0F2C, -0x04) = 0x00400000U;

    vol = (signed short)(D_800E6B2A - 0x320U);
    D_800E6B2A = (unsigned short)vol;
    if (vol < 0) {
        D_800E6B2A = 0;
    }

    vol = (signed short)D_800E6B2A;
    D_800F0F30 = (unsigned short)vol;
    D_800F0F32 = (unsigned short)vol;

    func_800C44A8(D_800F0F2C - 0x04);
}
