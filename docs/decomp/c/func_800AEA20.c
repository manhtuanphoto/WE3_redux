/*
 * func_800AEA20
 *
 * Purpose:
 *   PsyQ GPU primitive setup helper (e.g. SetDrawArea / SetDrawOffset /
 *   SetDrawEnv-family wrapper). When D_800CEAC6 >= 2, logs via D_800CEAC0.
 *   Invokes the GPU driver table callback at D_800CEABC + 0x2C with `(prim, arg)`,
 *   writes a 4-word GPU DMA packet header at D_800CEB84 linking to D_800CEB70,
 *   and links `prim`'s tag word (+0x00) to D_800CEB84.
 *
 * Arguments:
 *   prim (u8*) - destination GPU primitive buffer (tag word written at +0x00)
 *   arg  (u8*) - input parameter struct passed to driver callback
 *
 * Return value:
 *   None (void)
 *
 * Globals read/written:
 *   D_80014F90 (u8[], R)   - debug format/label string
 *   D_800CEAC6 (u8, R)     - GPU debug level
 *   D_800CEAC0 (u8*, R)    - debug logger callback
 *   D_800CEABC (u8*, R)    - pointer to GPU driver table (+0x2C callback)
 *   D_800CEB70 (u8[], R)   - target GPU primitive/tag address
 *   D_800CEB84 (u8[], W)   - intermediate 4-word GPU DMA tag linked from `prim`
 */

#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned char D_80014F90[];
extern unsigned char D_800CEAC6;
extern unsigned char D_800CEAC0[];
extern unsigned char D_800CEABC[];
extern unsigned char D_800CEB70[];
extern unsigned char D_800CEB84[];

void func_800AEA20(unsigned char *prim, unsigned char *arg)
{
    unsigned char *gpuDrv;
    void (*buildFn)(unsigned char *, unsigned char *);

    if (D_800CEAC6 >= 2) {
        ((void (*)(unsigned char *, unsigned char *, unsigned char *))W(D_800CEAC0, 0))(
            D_80014F90, prim, arg);
    }

    gpuDrv = (unsigned char *)W(D_800CEABC, 0);
    buildFn = (void (*)(unsigned char *, unsigned char *))W(gpuDrv, 0x2C);
    buildFn(prim, arg);

    W(D_800CEB84, 0x00) = ((unsigned int)D_800CEB70 & 0x00FFFFFFU) | 0x04000000U;
    W(prim, 0x00) = (unsigned int)D_800CEB84 & 0x00FFFFFFU;
}
