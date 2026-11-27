/*
 * func_800A98E0
 *
 * Purpose:
 *   Sets the stereo volume of SPU voice 23 (voice mask 0x00800000) from the
 *   lookup table D_800CE738 indexed by `vol_idx`. Records `vol_idx` in
 *   D_800E6AA2, configures the SPU voice attribute packet at D_800F0EE8 with
 *   mask 0x00800000, attribute flags 0x3 (left + right volume), and both
 *   D_800F0EF0 (left) and D_800F0EF2 (right) set to (D_800CE738[vol_idx] << 7),
 *   then submits it to func_800C44A8 (SpuSetVoiceAttr).
 *
 * Arguments:
 *   vol_idx (signed short) - index into D_800CE738 volume table
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800CE738 (u16[], R): volume lookup table
 *   D_800E6AA2 (s16, W):   stored volume index
 *   D_800F0EE8 (u8[], W):  SPU voice attribute struct (+0x00 mask = 0x00800000)
 *   D_800F0EEC (u32, W):   attribute flags = 3 (alias D_800F0EE8 + 0x04)
 *   D_800F0EF0 (u16, W):   left volume (alias D_800F0EE8 + 0x08)
 *   D_800F0EF2 (u16, W):   right volume (alias D_800F0EE8 + 0x0A)
 */

#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned short D_800CE738[];
extern signed short D_800E6AA2;
extern unsigned char D_800F0EE8[];
extern unsigned int D_800F0EEC;
extern unsigned short D_800F0EF0;
extern unsigned short D_800F0EF2;

extern void func_800C44A8(unsigned char *attr);

void func_800A98E0(signed short vol_idx)
{
    unsigned short scaled_vol = (unsigned short)(D_800CE738[vol_idx] << 7);

    D_800E6AA2 = vol_idx;
    W(D_800F0EE8, 0x00) = 0x00800000u;
    D_800F0EEC = 3u;
    D_800F0EF0 = scaled_vol;
    D_800F0EF2 = scaled_vol;

    func_800C44A8(D_800F0EE8);
}
