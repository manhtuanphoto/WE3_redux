/*
 * func_800AAAD8
 *
 * Purpose:
 *   Steps up SPU fade-in volume (`D_800E6B2A`) by +0x32 (50) per call until it
 *   reaches `D_800CE758[D_800E6AA0] << 7` (clamped, clearing `D_800E6B4C` to 0),
 *   then writes the volume to `D_800F0F30` / `D_800F0F32` and applies it via
 *   `func_800C44A8(D_800F0F28)`. If `D_8010A5D6 == 0`, resets `D_800E6B2A` to 0
 *   and calls `func_800AAC6C()` first.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_8010A5D6 (u8)    initialization/active flag
 *   D_800E6B2A (u16)   current fade volume
 *   D_800E6AA0 (s16)   master volume table index
 *   D_800CE758 (s16[]) target volume table (scaled << 7)
 *   D_800E6B4C (u16)   fade-in active flag (cleared to 0 when target reached)
 *   D_800F0F30 (u16)   SPU voice attr left volume (D_800F0F28 + 0x08)
 *   D_800F0F32 (u16)   SPU voice attr right volume (D_800F0F28 + 0x0A)
 */

#define H(p, o) (*(unsigned short *)((p) + (o)))

extern unsigned char D_8010A5D6;
extern unsigned short D_800E6B2A;
extern signed short D_800E6AA0;
extern signed short D_800CE758[];
extern unsigned short D_800E6B4C;
extern unsigned char D_800F0F30[];
extern unsigned short D_800F0F32;

extern void func_800AAC6C(void);
extern void func_800C44A8(unsigned char *ctx);

void func_800AAAD8(void)
{
    signed short target;
    unsigned short vol;

    if (D_8010A5D6 == 0) {
        D_800E6B2A = 0;
        func_800AAC6C();
    }

    D_800E6B2A = (unsigned short)(D_800E6B2A + 0x32U);
    target = D_800CE758[D_800E6AA0];
    if ((signed short)D_800E6B2A >= (target << 7)) {
        D_800E6B2A = (unsigned short)(target << 7);
        D_800E6B4C = 0;
    }

    vol = D_800E6B2A;
    H(D_800F0F30, 0x00) = vol;
    D_800F0F32 = vol;
    func_800C44A8(D_800F0F30 - 8);
}
