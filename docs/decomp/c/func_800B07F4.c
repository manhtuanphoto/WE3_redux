/*
 * func_800B07F4
 *
 * Purpose:
 *   Low-level GPU / DMA hardware reset helper (called by func_800AE1B8 / ResetGraph).
 *   Disables/saves interrupt state via func_800BA8B4, resets queue counters
 *   D_800CEBC0/D_800CEBBC, configures GPU/DMA control registers according to
 *   (mode & 7):
 *     - 0 or 5: resets DMA/GPU (*D_800CEBA8 = 0x401, *D_800CEBB8 |= 0x800,
 *       *D_800CEB9C = 0) and zeroes buffers D_800E7090 (0x100) and D_8010A9B8 (0x1800).
 *     - 1 or 3: resets DMA/GPU and writes GP1 reset commands 0x02000000 and
 *       0x01000000 to *D_800CEB9C.
 *   Restores interrupt state via func_800BA8B4(D_800CEBCC) and, if (mode & 7) == 0,
 *   tail-calls func_800B0BF8(mode).
 *
 * Arguments:
 *   mode (signed int): reset mode flags
 *
 * Return value:
 *   Return value of func_800B0BF8(mode) if (mode & 7) == 0; otherwise 0.
 *
 * Global fields read / written:
 *   D_800CEB9C (volatile u32 *): GPU GP1/control register pointer
 *   D_800CEBA8 (volatile u32 *): DMA control/channel register pointer
 *   D_800CEBB8 (volatile u32 *): DMA DPCR register pointer
 *   D_800CEBBC (u32): GPU queue read/head index
 *   D_800CEBC0 (u32): GPU queue write/tail index
 *   D_800CEBCC (u32): saved interrupt mask/state
 *   D_800E7090 (u8[0x100]), D_8010A9B8 (u8[0x1800]): GPU queue/command buffers
 */

extern volatile unsigned int *D_800CEB9C;
extern volatile unsigned int *D_800CEBA8;
extern volatile unsigned int *D_800CEBB8;
extern unsigned int D_800CEBBC;
extern unsigned int D_800CEBC0;
extern unsigned int D_800CEBCC;
extern unsigned char D_800E7090[];
extern unsigned char D_8010A9B8[];

extern unsigned int func_800BA8B4(unsigned int arg0);
extern void func_800B10D8(void *dst, signed int val, signed int size);
extern signed int func_800B0BF8(signed int mode);

signed int func_800B07F4(signed int mode)
{
    unsigned int submode;

    D_800CEBCC = func_800BA8B4(0);
    D_800CEBC0 = 0;
    D_800CEBBC = D_800CEBC0;

    submode = (unsigned int)mode & 7U;
    if (submode == 1 || submode == 3) {
        *D_800CEBA8 = 0x401U;
        *D_800CEBB8 |= 0x800U;
        *D_800CEB9C = 0x02000000U;
        *D_800CEB9C = 0x01000000U;
    } else if (submode == 0 || submode == 5) {
        *D_800CEBA8 = 0x401U;
        *D_800CEBB8 |= 0x800U;
        *D_800CEB9C = 0U;
        func_800B10D8(D_800E7090, 0, 0x100);
        func_800B10D8(D_8010A9B8, 0, 0x1800);
    }

    func_800BA8B4(D_800CEBCC);

    if (((unsigned int)mode & 7U) == 0) {
        return func_800B0BF8(mode);
    }

    return 0;
}
