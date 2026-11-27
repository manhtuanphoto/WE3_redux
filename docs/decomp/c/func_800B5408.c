/*
 * func_800B5408
 *
 * Purpose:
 *   Updates one directional light slot (0, 1, or 2) in the GTE light direction
 *   matrix D_800F0EC0 and the active GTE color matrix. Normalizes the light
 *   direction vector to 4.12 fixed-point (negated) and converts the 8-bit RGB
 *   color components to 4.12 fixed-point (val * 4096 / 255).
 *
 * Arguments:
 *   axisIndex - light slot index (0, 1, or 2)
 *   dirLight  - pointer to light descriptor (+0/+4/+8 s32 dir x/y/z, +0xC/+0xD/+0xE u8 r/g/b)
 *
 * Return value:
 *   0 on success, -1 if the direction vector has zero length.
 *
 * Reads / Writes:
 *   dirLight:
 *     +0x00/+0x04/+0x08 (s32) direction vector x/y/z (R)
 *     +0x0C/+0x0D/+0x0E (u8)  color r/g/b (0..255) (R)
 *   Globals:
 *     D_800F0EC0 (s16[3][3] + padding, 32 bytes): GTE light direction matrix (R/W)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   << 12 = 4096 (1.0 in PS1 GTE 4.12 fixed-point format)
 *   / 255 = normalizes 8-bit color channel (0..255) into 0..0x1000
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800F0EC0[];

extern void func_800B5924(signed short*);
extern int func_800B1128(int);
extern void func_800B58C0(signed short*);

int func_800B5408(int axisIndex, unsigned char* dirLight) {
    unsigned int r;
    unsigned int g;
    unsigned int b;
    unsigned int lightMatWords[8];
    signed short* lightMat;
    signed short colorMat[16];
    int len;
    int i;

    r = B(dirLight, 0xC);
    g = B(dirLight, 0xD);
    b = B(dirLight, 0xE);

    for (i = 0; i < 8; i++) {
        lightMatWords[i] = W(D_800F0EC0, i * 4);
    }
    lightMat = (signed short*)lightMatWords;

    func_800B5924(colorMat);

    len = func_800B1128(
        SW(dirLight, 0x0) * SW(dirLight, 0x0) +
        SW(dirLight, 0x4) * SW(dirLight, 0x4) +
        SW(dirLight, 0x8) * SW(dirLight, 0x8)
    );
    if (len == 0) {
        return -1;
    }

    switch (axisIndex) {
    case 0:
        lightMat[0] = (signed short)((-SW(dirLight, 0x0) << 12) / len);
        lightMat[1] = (signed short)((-SW(dirLight, 0x4) << 12) / len);
        lightMat[2] = (signed short)((-SW(dirLight, 0x8) << 12) / len);
        colorMat[0] = (signed short)((int)(r << 12) / 255);
        colorMat[3] = (signed short)((int)(g << 12) / 255);
        colorMat[6] = (signed short)((int)(b << 12) / 255);
        break;
    case 1:
        lightMat[3] = (signed short)((-SW(dirLight, 0x0) << 12) / len);
        lightMat[4] = (signed short)((-SW(dirLight, 0x4) << 12) / len);
        lightMat[5] = (signed short)((-SW(dirLight, 0x8) << 12) / len);
        colorMat[1] = (signed short)((int)(r << 12) / 255);
        colorMat[4] = (signed short)((int)(g << 12) / 255);
        colorMat[7] = (signed short)((int)(b << 12) / 255);
        break;
    case 2:
        lightMat[6] = (signed short)((-SW(dirLight, 0x0) << 12) / len);
        lightMat[7] = (signed short)((-SW(dirLight, 0x4) << 12) / len);
        lightMat[8] = (signed short)((-SW(dirLight, 0x8) << 12) / len);
        colorMat[2] = (signed short)((int)(r << 12) / 255);
        colorMat[5] = (signed short)((int)(g << 12) / 255);
        colorMat[8] = (signed short)((int)(b << 12) / 255);
        break;
    }

    for (i = 0; i < 8; i++) {
        W(D_800F0EC0, i * 4) = lightMatWords[i];
    }
    func_800B58C0(colorMat);
    return 0;
}
