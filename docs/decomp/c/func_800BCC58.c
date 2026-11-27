/*
 * func_800BCC58
 *
 * Purpose:
 *   Configures SPU common CD or external audio stereo volume via SpuSetCommonAttr
 *   (func_800C3FC8). Clamps `vol_l` and `vol_r` to 0x7F (127), scales each by 258
 *   (mapping 0x7F -> 0x7FFE, near 0x7FFF max SPU volume), populates a 0x28-byte
 *   SpuCommonAttr-like struct on the stack, and submits it to func_800C3FC8.
 *
 * Arguments:
 *   channel (unsigned char): 0 = CD volume (mask 0xC0, offsets +0x10/+0x12),
 *                            1 = external volume (mask 0xC00, offsets +0x1C/+0x1E)
 *   vol_l (signed short): left channel volume (0..0x7F)
 *   vol_r (signed short): right channel volume (0..0x7F)
 *
 * Return value:
 *   None (void)
 */

#define SH(p, o) (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))

extern void func_800C3FC8(unsigned char *attr);

void func_800BCC58(unsigned char channel, signed short vol_l, signed short vol_r)
{
    unsigned char attr[0x28];
    signed short l = vol_l;
    signed short r = vol_r;

    if ((channel & 0xFF) == 0) {
        W(attr, 0x00) = 0xC0;
        if (l >= 0x80) {
            l = 0x7F;
        }
        if (r >= 0x80) {
            r = 0x7F;
        }
        SH(attr, 0x10) = (signed short)(l * 258);
        SH(attr, 0x12) = (signed short)(r * 258);
    }

    if ((signed char)channel == 1) {
        W(attr, 0x00) = 0xC00;
        if (l >= 0x80) {
            l = 0x7F;
        }
        if (r >= 0x80) {
            r = 0x7F;
        }
        SH(attr, 0x1C) = (signed short)(l * 258);
        SH(attr, 0x1E) = (signed short)(r * 258);
    }

    /* UNCLEAR: if channel != 0 && (signed char)channel != 1, attr is uninitialized */
    func_800C3FC8(attr);
}
