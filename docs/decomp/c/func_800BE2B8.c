/*
 * func_800BE2B8
 *
 * Purpose:
 *   Updates slot/channel state tables (D_8010A3E0..D_8010A422), clears bit
 *   D_8010A3E2 in the 16-entry bitmask array D_8010CE28, initializes a 54-byte
 *   stride entry in D_800E78DE to 0x7FFF, looks up entries in D_800F1098 (16-byte
 *   stride) and D_800F20B0 (32-byte stride), and sets update flags (0x08 / 0x30)
 *   in D_800E81E0.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read / written:
 *   D_8010A3CF (s8), D_8010A3D4 (s8)
 *   D_8010A3E0 (u16), D_8010A3E2 (s16), D_8010A3E4 (s16), D_8010A3E6 (s16)
 *   D_800E78DE (s16 table, 54-byte stride indexed by D_8010A3E2)
 *   D_8010CE28 (u32[16] bitmask table)
 *   D_800F1098 (u8 * pointer to 16-byte struct array)
 *   D_800F20B0 (u8 * pointer to 32-byte struct array)
 *   D_800EF980 (u16)
 *   D_800E81E0 (u8[] flags)
 *   D_8010A41E, D_8010A420, D_8010A422 (u16 arrays indexed by s16 field)
 */

#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))

extern signed char D_8010A3CF;   /* D_8010A3E4 - 0x15 */
extern signed char D_8010A3D4;   /* D_8010A3E4 - 0x10 */
extern unsigned short D_8010A3E0;
extern short D_8010A3E2;         /* D_8010A3E4 - 0x02 */
extern short D_8010A3E4;
extern short D_8010A3E6;         /* D_8010A3E4 + 0x02 */
extern unsigned char D_800E78DE[];
extern unsigned int D_8010CE28[16];
extern unsigned char *D_800F1098;
extern unsigned char *D_800F20B0;
extern unsigned short D_800EF980;
extern unsigned char D_800E81E0[];
extern unsigned char D_8010A41E[];
extern unsigned char D_8010A420[];
extern unsigned char D_8010A422[];

void func_800BE2B8(void)
{
    short slot;
    short scaled;
    int idx;
    int i;
    unsigned short val;

    slot = D_8010A3E2;
    scaled = (short)(slot << 3);
    D_8010A3E4 = scaled;
    D_8010A3E6 = (short)((int)D_8010A3D4 + ((int)D_8010A3CF << 4));

    /* ((slot * 8 - slot) * 4 - slot) * 2 = slot * 54 */
    *(short *)(D_800E78DE + (int)slot * 54) = 0x7FFF;

    for (i = 0; i < 16; i++) {
        D_8010CE28[i] &= ~(1U << (unsigned int)(int)D_8010A3E2);
    }

    val = D_8010A3E0;
    idx = ((int)(short)val - 1) / 2;
    if ((val & 1) != 0) {
        *(unsigned short *)(D_8010A41E + (int)D_8010A3E4 * 2) =
            H(D_800F1098, idx * 16 + 0x0C);
    } else {
        *(unsigned short *)(D_8010A41E + (int)D_8010A3E4 * 2) =
            H(D_800F1098, idx * 16 + 0x0E);
    }

    D_800E81E0[D_8010A3E2] |= 0x08;

    idx = ((int)D_8010A3CF << 4) + (int)D_8010A3D4;
    *(unsigned short *)(D_8010A420 + (int)D_8010A3E4 * 2) =
        H(D_800F20B0, idx * 32 + 0x10);

    idx = ((int)D_8010A3CF << 4) + (int)D_8010A3D4;
    *(unsigned short *)(D_8010A422 + (int)D_8010A3E4 * 2) =
        (unsigned short)(H(D_800F20B0, idx * 32 + 0x12) + D_800EF980);

    D_800E81E0[D_8010A3E2] |= 0x30;
}
