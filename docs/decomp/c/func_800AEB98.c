/*
 * func_800AEB98
 *
 * Purpose:
 *   PsyQ PutDrawEnv equivalent. Optionally logs the DRAWENV struct via
 *   D_800CEAC0 when D_800CEAC6 >= 2, converts the DRAWENV into a DR_ENV GPU
 *   primitive packet at drawEnv + 0x1C via func_800AF590, marks the packet tag
 *   end-of-list (| 0x00FFFFFF), submits it through the GPU driver table
 *   D_800CEABC (+0x08 callback with arg0 = +0x18, length = 0x40), and caches
 *   the 0x5C-byte DRAWENV into D_800CEAC6 + 0x0E.
 *
 * Arguments:
 *   drawEnv (u8*) - pointer to 0x5C-byte DRAWENV struct (DR_ENV packet at +0x1C)
 *
 * Return value:
 *   u8* - returns drawEnv
 *
 * Globals read/written:
 *   D_80014FBC (u8[], R)   - debug format/label string
 *   D_800CEAC0 (u8*, R)    - debug logger function pointer
 *   D_800CEAC6 (u8[], R/W) - GPU debug level at +0x00; cached DRAWENV (0x5C bytes) at +0x0E
 *   D_800CEABC (u8*, R)    - pointer to GPU driver table (+0x08 fn, +0x18 ctx)
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned char D_80014FBC[];
extern unsigned char D_800CEABC[];
extern unsigned char D_800CEAC0[];
extern unsigned char D_800CEAC6[];

extern void func_800AF590(unsigned char *drEnv, unsigned char *drawEnv);
extern void func_800B1118(unsigned char *dst, unsigned char *src, int size);

unsigned char *func_800AEB98(unsigned char *drawEnv)
{
    unsigned char *gpuCtx = D_800CEAC6;
    unsigned char *gpuDrv;
    void (*submitFn)(unsigned int, unsigned char *, int, int);

    if (B(gpuCtx, 0) >= 2) {
        ((void (*)(unsigned char *, unsigned char *))W(D_800CEAC0, 0))(D_80014FBC, drawEnv);
    }

    func_800AF590(drawEnv + 0x1C, drawEnv);
    W(drawEnv, 0x1C) |= 0x00FFFFFFU;

    gpuDrv = (unsigned char *)W(D_800CEABC, 0);
    submitFn = (void (*)(unsigned int, unsigned char *, int, int))W(gpuDrv, 0x08);
    submitFn(W(gpuDrv, 0x18), drawEnv + 0x1C, 0x40, 0);

    func_800B1118(gpuCtx + 0x0E, drawEnv, 0x5C);
    return drawEnv;
}
