/*
 * func_8001F7C4
 *
 * Purpose:
 *   Configures stadium sky/backdrop/crowd layer objects via func_8001CEE0 and
 *   updates their active buffer bitmasks (D_80105D78, D_80105DA0, D_80105DC8)
 *   from `(1 << W(Scratchpad, 0)) - 1` based on Scratchpad+0x2E5 (time-of-day
 *   / weather) and Scratchpad+0x2E4 (sky/lighting sub-variant 0 or 1).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Scratchpad (0x1F800000) / Global fields read & written:
 *   Scratchpad +0x000 (u32) - buffer count / bit shift for mask `(1 << val) - 1`
 *   Scratchpad +0x2E4 (u8)  - sky/lighting variant (0 or 1)
 *   Scratchpad +0x2E5 (u8)  - time-of-day / weather flag (0 vs non-zero)
 *   D_80105D78        (u32) - layer 0x36 mask
 *   D_80105DA0        (u32) - layer 0x37 mask
 *   D_80105DA4        (u8)  - cleared to 0 when 0x2E4 == 1
 *   D_80105DC8        (u32) - layer 0x38 mask
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))
#define SCRATCHPAD ((unsigned char *)0x1F800000)

extern unsigned int D_80105D78;
extern unsigned int D_80105DA0;
extern unsigned char D_80105DA4;
extern unsigned int D_80105DC8;

extern void func_8001CEE0(int slot, int model_id, int flags, int a3, int a4, int a5, int a6, int a7);

void func_8001F7C4(void)
{
    unsigned char *scratch = SCRATCHPAD;
    unsigned char tod = B(scratch, 0x2E5);
    unsigned char var = B(scratch, 0x2E4);
    unsigned int mask;

    if (tod == 0) {
        if (var == 0) {
            func_8001CEE0(0x36, 0x28, 0, 0, -0x20, 0x1F, 0, 1);
            func_8001CEE0(0x37, 0x29, 0x01000000, 0, -0x20, 0x1E, 0, 1);
            func_8001CEE0(0x38, 0x2A, 0, 0, -0x20, 0x1F, 0, 1);
            mask = (1U << W(scratch, 0)) - 1U;
            D_80105D78 = mask;
            D_80105DA0 = mask;
            D_80105DC8 = mask;
        } else if (var == 1) {
            func_8001CEE0(0x36, 0x2B, 0x50000000, 0, 0, 0x1F, 0, 1);
            func_8001CEE0(0x38, 0x42, 0, 0, -0x20, 0x1F, 0, 1);
            D_80105DA4 = 0;
            D_80105DC8 = (1U << W(scratch, 0)) - 1U;
        }
    } else {
        if (var == 0) {
            func_8001CEE0(0x36, 0x43, 0, 0, -0x20, 0x0A, 0, 1);
            D_80105D78 = (1U << W(scratch, 0)) - 1U;
        } else if (var == 1) {
            func_8001CEE0(0x36, 0x2B, 0x50000000, 0, 0, 0x1F, 0, 1);
            func_8001CEE0(0x38, 0x43, 0, 0, -0x20, 0x0A, 0, 1);
            D_80105DA4 = 0;
            D_80105DC8 = (1U << W(scratch, 0)) - 1U;
        }
    }
}
