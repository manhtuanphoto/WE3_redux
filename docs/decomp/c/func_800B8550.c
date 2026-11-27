/*
 * func_800B8550
 *
 * Purpose:
 *   Low-level PS1 CD-ROM controller reset/acknowledge helper. Selects CD bank 1,
 *   drains/acknowledges any pending interrupt flags in the interrupt status register
 *   (*D_800D3C20 & 7) by writing 7 to *D_800D3C20 and *D_800D3C1C, resets
 *   D_800D3C2C[0..2] to {2, 0, 0}, selects bank 0, clears *D_800D3C20, and writes
 *   0x1325 to *D_800D3C24.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Globals read/written:
 *   D_800D3C14 (volatile u8*)  - pointer to CD-ROM HSTS / bank select register (0x1F801800)
 *   D_800D3C1C (volatile u8*)  - pointer to CD-ROM HCLRCTL / interrupt clear register
 *   D_800D3C20 (volatile u8*)  - pointer to CD-ROM HINTSTS / interrupt flag register
 *   D_800D3C24 (volatile u32*) - pointer to CD-ROM / DMA control register (written 0x1325)
 *   D_800D3C2C (u8[3])         - CD completion status buffer ({2, 0, 0})
 */

extern volatile unsigned char *D_800D3C14;
extern volatile unsigned char *D_800D3C1C;
extern volatile unsigned char *D_800D3C20;
extern volatile unsigned int  *D_800D3C24;
extern unsigned char D_800D3C2C[];

void func_800B8550(void)
{
    *D_800D3C14 = 1;
    if ((*D_800D3C20 & 7U) != 0) {
        do {
            *D_800D3C14 = 1;
            *D_800D3C20 = 7;
            *D_800D3C1C = 7;
        } while ((*D_800D3C20 & 7U) != 0);
    }

    D_800D3C2C[2] = 0;
    D_800D3C2C[1] = D_800D3C2C[2];
    D_800D3C2C[0] = 2;

    *D_800D3C14 = 0;
    *D_800D3C20 = 0;
    *D_800D3C24 = 0x1325U;
}
