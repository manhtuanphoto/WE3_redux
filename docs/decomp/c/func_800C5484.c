/*
 * func_800C5484
 *
 * Purpose:
 *   MDEC / DecDCT timeout or error recovery handler (PsyQ library routine).
 *   Prints DMA channel and MDEC status register bits via func_800AD698 (printf),
 *   resets the MDEC control register (*D_800D5A40 = 0x80000000), clears the two
 *   DMA channel control registers (*D_800D5A14 = 0, *D_800D5A20 = 0), and
 *   re-enables MDEC DMA (*D_800D5A40 = 0x60000000).
 *
 *   Note: func_800C5484.s is an unsplit dlabel block whose instructions for
 *   func_800C5484 span 0x800C5484..0x800C55AC. Trailing words at 0x800C55B8..
 *   0x800C5994 (separate MDEC VLC decoder, BIOS A0 stubs, etc.) are unreachable
 *   from func_800C5484 and are not part of this function.
 *
 * Arguments:
 *   arg0 (unsigned int) - error/timeout label argument passed to func_800AD698
 *
 * Return value:
 *   0 (int)
 *
 * Global pointers read/written:
 *   D_800D5A40 (volatile u32 *) - MDEC status/control register pointer (read, written 0x80000000 then 0x60000000)
 *   D_800D5A14 (volatile u32 *) - DMA channel 0 (MDECin) CHCR pointer (read, cleared to 0)
 *   D_800D5A20 (volatile u32 *) - DMA channel 1 (MDECout) CHCR pointer (read, cleared to 0, dummy-read)
 *   D_800D5A0C (volatile u32 *) - DMA MADR pointer (read)
 *   D_800D5A18 (volatile u32 *) - DMA MADR pointer (read)
 *
 * Scratchpad fields read/written:
 *   None.
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   Hardware/PsyQ MDEC routine; no gameplay constants.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern volatile unsigned int *D_800D5A40;
extern volatile unsigned int *D_800D5A14;
extern volatile unsigned int *D_800D5A20;
extern volatile unsigned int *D_800D5A0C;
extern volatile unsigned int *D_800D5A18;

extern void func_800AD698(const char *fmt, ...);

int func_800C5484(unsigned int arg0)
{
    unsigned int mdec_stat;
    unsigned int chcr0;
    unsigned int chcr1;
    unsigned int madr0;
    unsigned int madr1;

    func_800AD698((const char *)0x80015688, arg0);

    mdec_stat = *D_800D5A40;
    chcr0 = *D_800D5A14;
    chcr1 = *D_800D5A20;
    madr0 = *D_800D5A0C;
    madr1 = *D_800D5A18;
    func_800AD698(
        (const char *)0x80015628,
        (chcr0 >> 24) & 1U,
        (chcr1 >> 24) & 1U,
        madr0,
        madr1
    );

    func_800AD698(
        (const char *)0x80015650,
        (~mdec_stat) >> 31,
        (mdec_stat >> 30) & 1U,
        (mdec_stat >> 29) & 1U,
        (mdec_stat >> 28) & 1U,
        (mdec_stat >> 27) & 1U,
        (mdec_stat >> 25) & 1U,
        (mdec_stat >> 23) & 1U
    );

    *D_800D5A40 = 0x80000000U;
    *D_800D5A14 = 0;
    *D_800D5A20 = 0;
    (void)*D_800D5A20;
    *D_800D5A40 = 0x60000000U;

    return 0;
}
