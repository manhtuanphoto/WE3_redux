/*
 * func_800B8940
 *
 * Purpose:
 *   Waits for or polls completion of an asynchronous CD/hardware operation by
 *   checking bit 24 (0x01000000) of *D_800D3C58 (analogous to PsyQ CdSync).
 *   Times out after 0x3C0 ticks (via func_800BA518) or 0x3C0001 iterations,
 *   logging diagnostic info via func_800AD698 and resetting via func_800B8550.
 *
 * Arguments:
 *   mode: (int) 0 = blocking wait until complete or timeout,
 *               nonzero = non-blocking poll (returns 1 if still busy)
 *
 * Return value:
 *   (int) 0 if complete, 1 if busy (when mode != 0), -1 on timeout
 *
 * Reads / Writes:
 *   D_800E71E0: (s32) deadline tick (current + 0x3C0) (R/W)
 *   D_800E71E4: (s32) poll iteration counter (limit 0x3C0000) (R/W)
 *   D_800E71E8: (void*) timeout context/label pointer set to D_800153D0 (R/W)
 *   D_800D3C58: (volatile u32**) status register pointer (bit 0x01000000 = busy) (R)
 *   D_800D396D, D_800D3974, D_800D39F4, D_800D3C2C: CD status/cmd diagnostic tables (R)
 */

extern unsigned char D_800152C8[];
extern unsigned char D_800152D8[];
extern unsigned char D_800153D0[];
extern unsigned char D_800D396D;
extern unsigned int D_800D3974[];
extern unsigned int D_800D39F4[];
extern unsigned char D_800D3C2C[];
extern volatile unsigned int *D_800D3C58;
extern int D_800E71E0;
extern int D_800E71E4;
extern void *D_800E71E8;

extern void func_800AD698(void *fmt, ...);
extern void func_800B8550(void);
extern void func_800B8D80(void *msg);
extern int func_800BA518(int arg);

int func_800B8940(int mode) {
    D_800E71E0 = func_800BA518(-1) + 0x3C0;
    D_800E71E4 = 0;
    D_800E71E8 = D_800153D0;

    while (1) {
        int now = func_800BA518(-1);
        int iter;

        if (D_800E71E0 < now || (iter = D_800E71E4, D_800E71E4 = iter + 1, iter > 0x3C0000)) {
            func_800B8D80(D_800152C8);
            func_800AD698(
                D_800152D8,
                D_800E71E8,
                D_800D3974[D_800D396D],
                D_800D39F4[D_800D3C2C[0]],
                D_800D39F4[D_800D3C2C[1]]
            );
            func_800B8550();
            return -1;
        }

        if ((*D_800D3C58 & 0x01000000U) == 0) {
            return 0;
        }

        if (mode != 0) {
            return 1;
        }
    }
}
