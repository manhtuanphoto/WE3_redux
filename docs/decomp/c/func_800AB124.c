/*
 * func_800AB124
 *
 * Purpose:
 *   Updates CD stereo volume (SPU common CD volume via func_800BCC58) by
 *   looking up a scale factor from D_800CE738[vol_idx], storing it in
 *   D_800E6B6C, scaling base left/right volumes D_800E6AD6 and D_800E6ADC
 *   by (scale / 128), and calling func_800BCC58(0, left_vol, right_vol).
 *
 * Arguments:
 *   vol_idx (signed short) - index into D_800CE738 volume scale table
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800CE738 (u16[], R): volume scale lookup table
 *   D_800E6AD6 (s16, R):   base left CD volume
 *   D_800E6ADC (s16, R):   base right CD volume
 *   D_800E6B6C (u16, W):   active volume scale factor
 */

extern unsigned short D_800CE738[];
extern signed short D_800E6AD6;
extern signed short D_800E6ADC;
extern unsigned short D_800E6B6C;

extern void func_800BCC58(unsigned char channel, signed short vol_l, signed short vol_r);

void func_800AB124(signed short vol_idx)
{
    unsigned short raw_scale = D_800CE738[vol_idx];
    signed short scale = (signed short)raw_scale;
    signed short left_vol;
    signed short right_vol;

    D_800E6B6C = raw_scale;

    left_vol = (signed short)(((signed int)D_800E6AD6 * (signed int)scale) / 128);
    right_vol = (signed short)(((signed int)D_800E6ADC * (signed int)scale) / 128);

    func_800BCC58(0, left_vol, right_vol);
}
