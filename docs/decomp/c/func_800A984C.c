/*
 * func_800A984C
 *
 * Purpose:
 *   Configures SPU/audio parameter struct at D_800F0EE8 using preset index
 *   `idx` (stored to D_800E6AA2). Sets D_800F0EE8 = 0x00800000 (attribute mask),
 *   D_800F0EEC = 3, and both D_800F0EF0 and D_800F0EF2 (L/R volume halfwords)
 *   to (D_800CE738[idx] << 7), then applies the struct via func_800C44A8(&D_800F0EE8)
 *   and calls func_800AB240(idx).
 *
 * Arguments:
 *   idx (signed short): preset/volume table index
 *
 * Return value:
 *   None (void)
 *
 * Globals read / written:
 *   D_800E6AA2 (s16, W): saved preset index
 *   D_800CE738 (u16[], R): volume lookup table (scaled << 7)
 *   D_800F0EE8 (u32, W): attribute mask (0x00800000)
 *   D_800F0EEC (u32, W): mode field (3)
 *   D_800F0EF0 (s16, W): left volume (D_800CE738[idx] << 7)
 *   D_800F0EF2 (s16, W): right volume (D_800CE738[idx] << 7)
 */

extern signed short D_800E6AA2;
extern unsigned short D_800CE738[];
extern unsigned int D_800F0EE8;
extern unsigned int D_800F0EEC;
extern signed short D_800F0EF0;
extern signed short D_800F0EF2;

extern void func_800C44A8(unsigned int *attr);
extern void func_800AB240(signed int idx);

void func_800A984C(signed short idx)
{
    signed short vol;

    D_800E6AA2 = idx;
    D_800F0EE8 = 0x00800000U;
    D_800F0EEC = 3U;

    vol = (signed short)(D_800CE738[idx] << 7);
    D_800F0EF0 = vol;
    D_800F0EF2 = vol;

    func_800C44A8(&D_800F0EE8);
    func_800AB240((signed int)idx);
}
