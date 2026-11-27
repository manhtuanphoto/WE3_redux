/*
 * func_800AFF50
 *
 * Purpose:
 *   PS1 GPU VRAM-to-RAM transfer (`StoreImage` from libgpu). Syncs the GPU via
 *   func_800B0A80, clamps the rectangle width/height against D_800CEAC8/CEACA,
 *   computes 32-bit word count `(w * h + 1) / 2` split into 16-word DMA blocks
 *   and remainder words, issues GP0 command `0xC0000000` (VRAM->CPU transfer)
 *   to *D_800CEB98 after polling GPUSTAT bit 26 (`0x04000000`), polls bit 27
 *   (`0x08000000`) for data ready, reads remainder words directly from GPUREAD
 *   (*D_800CEB98), and transfers remaining 16-word blocks via GPU DMA2
 *   (*D_800CEBA0/BA4/BA8).
 *
 * Arguments:
 *   rect (signed short *) - PS1 RECT {x, y, w, h} (w/h clamped in place)
 *   dst  (unsigned int *) - destination buffer in main RAM
 *
 * Return value:
 *   int - 0 on success, -1 on invalid size (<= 0 words) or GPU sync timeout.
 *
 * Hardware / Global pointers:
 *   D_800CEAC8 (s16)   - max VRAM width clamp
 *   D_800CEACA (s16)   - max VRAM height clamp
 *   D_800CEB98 (u32 *) - GPU0 / GPUREAD data register (0x1F801810)
 *   D_800CEB9C (u32 *) - GPU1 / GPUSTAT register (0x1F801814)
 *   D_800CEBA0 (u32 *) - DMA2 MADR (0x1F8010A0)
 *   D_800CEBA4 (u32 *) - DMA2 BCR (0x1F8010A4)
 *   D_800CEBA8 (u32 *) - DMA2 CHCR (0x1F8010A8)
 */

extern short D_800CEAC8;
extern short D_800CEACA;
extern volatile unsigned int *D_800CEB98;
extern volatile unsigned int *D_800CEB9C;
extern volatile unsigned int *D_800CEBA0;
extern volatile unsigned int *D_800CEBA4;
extern volatile unsigned int *D_800CEBA8;

extern void func_800B0A80(void);
extern int func_800B0AB4(void);

int func_800AFF50(signed short *rect, unsigned int *dst)
{
    int w;
    int h;
    int words;
    int blocks;
    int rem;

    func_800B0A80();

    w = rect[2];
    if (w < 0) {
        w = 0;
    } else if (D_800CEAC8 < w) {
        w = (unsigned short)D_800CEAC8;
    }
    rect[2] = (signed short)w;

    h = rect[3];
    if (h < 0) {
        h = 0;
    } else if (D_800CEACA < h) {
        h = (unsigned short)D_800CEACA;
    }
    rect[3] = (signed short)h;

    words = ((signed short)rect[2] * (signed short)rect[3] + 1) / 2;
    if (words <= 0) {
        return -1;
    }

    blocks = (rect[2] * rect[3] + 1) >> 5;
    rem = words - (blocks << 4);

    while (((*D_800CEB9C) & 0x04000000U) == 0) {
        if (func_800B0AB4() != 0) {
            return -1;
        }
    }

    *D_800CEB9C = 0x04000000U;
    *D_800CEB98 = 0x01000000U;
    *D_800CEB98 = 0xC0000000U;
    *D_800CEB98 = ((unsigned int *)rect)[0];
    *D_800CEB98 = ((unsigned int *)rect)[1];

    while (((*D_800CEB9C) & 0x08000000U) == 0) {
        if (func_800B0AB4() != 0) {
            return -1;
        }
    }

    rem--;
    while (rem != -1) {
        *dst++ = *D_800CEB98;
        rem--;
    }

    if (blocks != 0) {
        *D_800CEB9C = 0x04000003U;
        *D_800CEBA0 = (unsigned int)dst;
        *D_800CEBA4 = ((unsigned int)blocks << 16) | 0x10U;
        *D_800CEBA8 = 0x01000200U;
    }

    return 0;
}
