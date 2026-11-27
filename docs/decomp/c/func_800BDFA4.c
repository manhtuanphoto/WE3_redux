/*
 * func_800BDFA4
 *
 * Purpose:
 *   Sets the left and right volumes for one of 24 SPU/audio voices
 *   (indices 0..23). Scales each 16-bit input volume by 129 ((v << 7) + v)
 *   to map 0..127 into 0..0x3FFF, writes them to the 16-byte-per-voice
 *   attribute table at D_8010A418 / D_8010A41A, and sets bits 0..1 (0x03)
 *   in the per-voice dirty mask D_800E81E0[voice].
 *
 * Arguments:
 *   voice (signed short) - voice index (must satisfy (unsigned short)voice < 24)
 *   vol_l (signed short) - left volume (scaled by 129)
 *   vol_r (signed short) - right volume (scaled by 129)
 *
 * Return value:
 *   0 on success; -1 if (unsigned short)voice >= 24.
 *
 * Global fields read/written:
 *   D_8010A418 (s16[]) - voice left volume table (stride 0x10 bytes / 8 shorts)
 *   D_8010A41A (s16[]) - voice right volume table (stride 0x10 bytes / 8 shorts)
 *   D_800E81E0 (u8[])  - per-voice attribute update flags (ORed with 0x03)
 */

#define H(p, o) (*(signed short *)((p) + (o)))

extern unsigned char D_8010A418[];
extern unsigned char D_8010A41A[];
extern unsigned char D_800E81E0[];

int func_800BDFA4(signed short voice, signed short vol_l, signed short vol_r)
{
    int idx;
    int off;

    if ((unsigned short)voice >= 0x18U) {
        return -1;
    }

    idx = (int)voice;
    off = idx << 4;

    H(D_8010A41A, off) = (signed short)(((int)vol_r << 7) + (int)vol_r);
    H(D_8010A418, off) = (signed short)(((int)vol_l << 7) + (int)vol_l);
    D_800E81E0[idx] |= 0x03U;

    return 0;
}
