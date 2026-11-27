/*
 * func_800AFA04 - Clear an Ordering Table in reverse via DMA6 OTC (ClearOTagR)
 *
 * Purpose:
 *   Enables DMA channel 6 (OTC) in DPCR (*D_800CEBB8 |= 0x08000000), clears
 *   CHCR (*D_800CEBB4), programs MADR (*D_800CEBAC) to the last entry
 *   `ot + (count - 1) * 4`, sets BCR (*D_800CEBB0) to `count`, starts reverse
 *   OTC DMA (*D_800CEBB4 = 0x11000002), calls func_800B0A80(), and polls the
 *   busy bit (0x01000000) until completion or until func_800B0AB4() signals abort.
 *
 * Arguments:
 *   ot (unsigned int):    base address of ordering table buffer
 *   count (unsigned int): number of 32-bit OT entries
 *
 * Return value:
 *   (signed int) `count` on completion, or -1 if func_800B0AB4() returns non-zero
 *
 * Globals read:
 *   D_800CEBAC (volatile u32 *): DMA6 MADR (0x1F8010E0)
 *   D_800CEBB0 (volatile u32 *): DMA6 BCR  (0x1F8010E4)
 *   D_800CEBB4 (volatile u32 *): DMA6 CHCR (0x1F8010E8)
 *   D_800CEBB8 (volatile u32 *): DPCR      (0x1F8010F0)
 */

extern volatile unsigned int *D_800CEBAC;
extern volatile unsigned int *D_800CEBB0;
extern volatile unsigned int *D_800CEBB4;
extern volatile unsigned int *D_800CEBB8;

extern void func_800B0A80(void);
extern int func_800B0AB4(void);

signed int func_800AFA04(unsigned int ot, unsigned int count)
{
    *D_800CEBB8 |= 0x08000000u;
    *D_800CEBB4 = 0u;
    *D_800CEBAC = ot + (count * 4u - 4u);
    *D_800CEBB0 = count;
    *D_800CEBB4 = 0x11000002u;

    func_800B0A80();

    while ((*D_800CEBB4 & 0x01000000u) != 0u) {
        if (func_800B0AB4() != 0) {
            return -1;
        }
    }

    return (signed int)count;
}
