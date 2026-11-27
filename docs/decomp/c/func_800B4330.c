/*
 * func_800B4330
 *
 * Purpose:
 *   Initializes 3D projection/viewport matrices and aspect-ratio scaling state
 *   (likely GsInit3D / SetGeomScreen-like setup). Stores width (a0) and height (a1)
 *   in D_800FB57C and D_800FB5AC, sets up an identity 32-byte MATRIX at
 *   (D_800FB5C0 - 0x10), copies it to D_800FF708, D_800F0EC0 (zeroing diagonal),
 *   and D_800F1078, sets Y scale D_800FF708[0x08] = ((height << 14) / width) / 3,
 *   clears several 2-short globals, sets D_800E7198 flags, and sets D_8010CD50 = 1.
 *
 * Arguments:
 *   width (unsigned short): screen/viewport width
 *   height (unsigned short): screen/viewport height
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800FB57C (u32), D_800FB5AC (u32),
 *   D_800FB5C0 (-0x10..+0x0F, 32-byte MATRIX),
 *   D_800FF708 (32-byte MATRIX, +0x08 aspect Y scale),
 *   D_800F0EC0 (32-byte MATRIX), D_800F1078 (32-byte MATRIX),
 *   D_800E81F8, D_800E8200, D_800EF868, D_800FF738 (s16/u16 structs),
 *   D_800E7198 (u8[]), D_8010CD50 (u32) = 1
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define H(p, o) (*(unsigned short *)((p) + (o)))
#define S(p, o) (*(signed short *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned int  D_800FB57C;
extern unsigned int  D_800FB5AC;
extern unsigned char D_800FB5C0[]; /* base MATRIX starts at D_800FB5C0 - 0x10 */
extern unsigned char D_800FF708[];
extern unsigned char D_800F0EC0[];
extern unsigned char D_800F1078[];
extern unsigned char D_800E81F8[];
extern unsigned char D_800E8200[];
extern unsigned char D_800EF868[];
extern unsigned char D_800FF738[];
extern unsigned char D_800E7198[];
extern unsigned int  D_8010CD50;

void func_800B4330(unsigned short width, unsigned short height)
{
    unsigned char *baseMtx = D_800FB5C0 - 0x10;
    signed int ratio;
    int i;

    D_800FB57C = (unsigned int)(width & 0xFFFF);
    D_800FB5AC = (unsigned int)(height & 0xFFFF);

    ratio = (signed int)(D_800FB5AC << 14) / (signed int)D_800FB57C;

    /* Initialize identity MATRIX (3x3 4.12 fixed-point + 3x s32 translation) */
    S(baseMtx, 0x04) = 0;
    S(baseMtx, 0x02) = 0;
    S(baseMtx, 0x0A) = 0;
    S(baseMtx, 0x06) = 0;
    S(baseMtx, 0x0E) = 0;
    S(baseMtx, 0x0C) = 0;
    W(baseMtx, 0x1C) = 0;
    W(baseMtx, 0x18) = 0;
    W(baseMtx, 0x14) = 0;
    S(baseMtx, 0x00) = 0x1000;
    S(baseMtx, 0x08) = 0x1000;
    S(baseMtx, 0x10) = 0x1000;

    for (i = 0; i < 32; i += 4) {
        W(D_800FF708, i) = W(baseMtx, i);
    }

    for (i = 0; i < 32; i += 4) {
        W(D_800F0EC0, i) = W(baseMtx, i);
    }
    S(D_800F0EC0, 0x10) = 0;
    S(D_800F0EC0, 0x08) = 0;
    S(D_800F0EC0, 0x00) = 0;

    for (i = 0; i < 32; i += 4) {
        W(D_800F1078, i) = W(D_800F0EC0, i);
    }

    H(D_800E81F8, 0) = 0;
    H(D_800E81F8, 2) = 0;
    H(D_800E8200, 0) = 0;
    H(D_800E8200, 2) = 0;
    H(D_800EF868, 2) = 0;
    H(D_800EF868, 0) = 0;
    H(D_800FF738, 2) = 0;
    H(D_800FF738, 0) = 0;

    S(D_800FF708, 0x08) = (signed short)(ratio / 3);

    B(D_800E7198, 0x03) = 3;
    B(D_800E7198, 0x07) = 2;
    B(D_800E7198, 0x13) = 3;
    B(D_800E7198, 0x17) = 2;

    D_8010CD50 = 1;
    H(D_800FF738, 4) = (unsigned short)D_800FB57C;
    H(D_800FF738, 6) = (unsigned short)D_800FB5AC;
}
