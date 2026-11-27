/*
 * func_800B0944
 *
 * Purpose:
 *   GPU drawing queue synchronization (PsyQ DrawSync-style routine).
 *   When mode == 0 (blocking), calls func_800B0A80(), drains the draw queue
 *   (D_800CEBBC != D_800CEBC0) via func_800B0594() while checking func_800B0AB4()
 *   for timeout/error, then waits until (*D_800CEBA8 & 0x01000000) == 0 and
 *   (*D_800CEB9C & 0x04000000) != 0, returning 0 (or -1 on error).
 *   When mode != 0 (non-blocking), steps func_800B0594() once if queue count
 *   ((D_800CEBBC - D_800CEBC0) & 0x3F) is non-zero, and returns that count
 *   (or 1 if count == 0 and GPU/DMA is still busy).
 *
 * Arguments:
 *   mode (int): 0 = wait until complete; non-zero = poll/return remaining count
 *
 * Return value:
 *   mode == 0: 0 on completion, -1 if func_800B0AB4() returns non-zero.
 *   mode != 0: remaining queue depth (0..63), or 1 if depth == 0 while GPU busy.
 *
 * Global fields read:
 *   D_800CEBBC (unsigned int): draw queue head/write index
 *   D_800CEBC0 (unsigned int): draw queue tail/read index
 *   D_800CEBA8 (volatile unsigned int *): status register pointer (bit 24 = 0x01000000)
 *   D_800CEB9C (volatile unsigned int *): status register pointer (bit 26 = 0x04000000)
 */

extern unsigned int D_800CEBBC;
extern unsigned int D_800CEBC0;
extern volatile unsigned int *D_800CEBA8;
extern volatile unsigned int *D_800CEB9C;

extern void func_800B0A80(void);
extern void func_800B0594(void);
extern int func_800B0AB4(void);

int func_800B0944(int mode)
{
    unsigned int rem;

    if (mode == 0) {
        func_800B0A80();

        while (D_800CEBBC != D_800CEBC0) {
            func_800B0594();
            if (func_800B0AB4() != 0) {
                return -1;
            }
        }

        while ((*D_800CEBA8 & 0x01000000u) != 0 || (*D_800CEB9C & 0x04000000u) == 0) {
            if (func_800B0AB4() != 0) {
                return -1;
            }
        }

        return 0;
    }

    rem = (D_800CEBBC - D_800CEBC0) & 0x3Fu;
    if (rem != 0) {
        func_800B0594();
    }

    if ((*D_800CEBA8 & 0x01000000u) != 0 || (*D_800CEB9C & 0x04000000u) == 0) {
        return (rem != 0) ? (int)rem : 1;
    }

    return (int)rem;
}
