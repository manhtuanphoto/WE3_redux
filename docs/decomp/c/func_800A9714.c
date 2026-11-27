/*
 * func_800A9714
 *
 * Purpose:
 *   Sets the master sound volume level index D_800E6AA0 to `level`, updates
 *   the channel volume table via func_800AB080(), configures SPU voice 22
 *   (mask 0x00400000) left/right volume (D_800CE758[level] << 7) in
 *   D_800F0F2C - 4 / D_800E6BD2, and applies it via func_800C44A8().
 *   (Identical to func_800A97A0 without the D_800E6BD8 / func_800AB240 call.)
 *
 * Arguments:
 *   level (signed short) - volume table index
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800E6AA0 (s16)   - master volume table index
 *   D_800CE758 (u16[]) - volume lookup table (shifted << 7 for SPU range)
 *   D_800F0F2C (u32)   - SPU voice attr flags at D_800F0F2C+0x00 (= 3)
 *   D_800F0F28 (u32)   - SPU voice mask at D_800F0F2C-0x04 (= 0x00400000, voice 22)
 *   D_800F0F30 (u16)   - left volume
 *   D_800F0F32 (u16)   - right volume
 *   D_800E6BD2 (u16)   - cached volume
 */

#define W(p, o) (*(unsigned int *)((p) + (o)))

extern signed short D_800E6AA0;
extern unsigned char D_800F0F2C[];
extern unsigned short D_800CE758[];
extern unsigned short D_800F0F30;
extern unsigned short D_800F0F32;
extern unsigned short D_800E6BD2;

extern void func_800AB080(void);
extern void func_800C44A8(unsigned char *attr);

void func_800A9714(signed short level)
{
    int idx = (int)level;
    unsigned short vol;

    D_800E6AA0 = level;
    func_800AB080();

    W(D_800F0F2C, 0x00) = 3U;
    W(D_800F0F2C, -0x04) = 0x00400000U;

    vol = (unsigned short)(D_800CE758[idx] << 7);
    D_800F0F30 = vol;
    D_800F0F32 = vol;
    D_800E6BD2 = vol;

    func_800C44A8(D_800F0F2C - 4);
}
