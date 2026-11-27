/*
 * func_800B0AB4
 *
 * Purpose:
 *   GPU DrawSync timeout check and recovery helper. Compares current VSync
 *   against deadline D_800CEBD0 and increments spin counter D_800CEBD4 (limit
 *   0xF0000). While within limits, returns 0. On timeout, logs GPU/DMA state via
 *   func_800AD698, resets GPU/DMA registers (*D_800CEBA8 = 0x401, *D_800CEBB8 |=
 *   0x800, *D_800CEB9C = 0x02000000 / 0x01000000), clears queue pointers
 *   D_800CEBC0/D_800CEBBC under interrupt lock (func_800BA8B4), and returns -1.
 *
 * Arguments:
 *   void
 *
 * Return value:
 *   0 while still waiting within timeout;
 *   -1 on timeout after resetting GPU/DMA.
 *
 * Global fields read / written:
 *   D_800CEBD0 (s32): VSync timeout threshold
 *   D_800CEBD4 (s32): spin iteration counter
 *   D_800CEB9C (volatile u32 *): GPU GP1/status register pointer
 *   D_800CEBA0 (volatile u32 *): DMA register pointer
 *   D_800CEBA8 (volatile u32 *): DMA control register pointer
 *   D_800CEBB8 (volatile u32 *): DMA DPCR register pointer
 *   D_800CEBBC (u32), D_800CEBC0 (u32): GPU queue head/tail indices
 *   D_800CEBCC (u32): saved interrupt mask/state
 *   D_80015008 (char[]): timeout format string
 */

extern signed int D_800CEBD0;
extern signed int D_800CEBD4;
extern volatile unsigned int *D_800CEB9C;
extern volatile unsigned int *D_800CEBA0;
extern volatile unsigned int *D_800CEBA8;
extern volatile unsigned int *D_800CEBB8;
extern unsigned int D_800CEBBC;
extern unsigned int D_800CEBC0;
extern unsigned int D_800CEBCC;
extern char D_80015008[];

extern signed int func_800BA518(signed int mode);
extern void func_800AD698(void *fmt, ...);
extern unsigned int func_800BA8B4(unsigned int arg0);

signed int func_800B0AB4(void)
{
    signed int vsync = func_800BA518(-1);

    if (vsync <= D_800CEBD0) {
        signed int cnt = D_800CEBD4;
        D_800CEBD4 = cnt + 1;
        if (cnt <= 0xF0000) {
            return 0;
        }
    }

    func_800AD698(
        D_80015008,
        (D_800CEBBC - D_800CEBC0) & 0x3FU,
        *D_800CEB9C,
        *D_800CEBA8,
        *D_800CEBA0
    );

    D_800CEBCC = func_800BA8B4(0);
    D_800CEBC0 = 0;
    D_800CEBBC = D_800CEBC0;

    *D_800CEBA8 = 0x401U;
    *D_800CEBB8 |= 0x800U;
    *D_800CEB9C = 0x02000000U;
    *D_800CEB9C = 0x01000000U;

    func_800BA8B4(D_800CEBCC);
    return -1;
}
