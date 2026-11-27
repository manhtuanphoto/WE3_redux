/*
 * func_800BAF78
 *
 * Purpose:
 *   Initializes PS1 DMA interrupt dispatching (equivalent to libetc/libapi
 *   DMA callback setup).
 *   - func_800BAF78: clears the 8-entry callback table D_800D4EC8 via
 *     func_800BB1F0, clears *D_800D4EC4 (DMA DICR at 0x1F8010F4), registers
 *     D_800BAFC4 as IRQ 3 (DMA) handler via func_800BA768, and returns a
 *     pointer to D_800BB144 (the per-channel callback setter).
 *   - D_800BAFC4: DMA IRQ dispatcher. Loops over pending channel IRQ flags
 *     (bits [30:24] of *D_800D4EC4), acknowledges each active channel bit in
 *     DICR, and invokes D_800D4EC8[ch](). If a DMA bus error occurs
 *     (bits [31:24] == 0x80000000 or bit 15 set), prints diagnostics via
 *     func_800AD698 using D_800D4EE8 (DMA channel MADR table, stride 0x10).
 *   - D_800BB144: sets or clears the callback for DMA channel `ch` (0..6) in
 *     D_800D4EC8 and updates channel IRQ enable bit `(1 << (ch + 16))` and
 *     master DMA IRQ enable bit 23 (`0x00800000`) in *D_800D4EC4, returning the
 *     previous callback pointer.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   void * - pointer to callback registration function D_800BB144.
 *
 * Hardware / Global fields:
 *   D_800D4EC4 (u32 *)       - pointer to DMA Interrupt Control Register (DICR, 0x1F8010F4)
 *   D_800D4EC8 (void (*[8])(void)) - DMA channel callback table
 *   D_800D4EE8 (u32 *)       - pointer to DMA channel registers (stride 0x10 bytes)
 *   D_800154CC, D_800154E8   - printf format strings for DMA error logging
 */

typedef void (*dma_cb_t)(void);

extern volatile unsigned int *D_800D4EC4;
extern dma_cb_t D_800D4EC8[8];
extern unsigned char *D_800D4EE8;
extern char D_800154CC[];
extern char D_800154E8[];

extern void func_800BB1F0(dma_cb_t *table, int count);
extern void func_800BA768(int irq, void (*handler)(void));
extern void func_800AD698(const char *fmt, ...);

static void D_800BAFC4(void);
static dma_cb_t D_800BB144(int ch, dma_cb_t cb);

void *func_800BAF78(void)
{
    func_800BB1F0(D_800D4EC8, 8);
    *D_800D4EC4 = 0;
    func_800BA768(3, D_800BAFC4);
    return (void *)D_800BB144;
}

static void D_800BAFC4(void)
{
    unsigned int flags;
    unsigned int dicr;
    int ch;

    flags = (*D_800D4EC4 >> 24) & 0x7FU;
    while (flags != 0) {
        ch = 0;
        while (flags != 0 && ch < 7) {
            if ((flags & 1U) != 0) {
                *D_800D4EC4 &= (1U << (ch + 24)) | 0x00FFFFFFU;
                if (D_800D4EC8[ch] != 0) {
                    D_800D4EC8[ch]();
                }
            }
            flags >>= 1;
            ch++;
        }
        flags = (*D_800D4EC4 >> 24) & 0x7FU;
    }

    dicr = *D_800D4EC4;
    if ((dicr & 0xFF000000U) == 0x80000000U || (dicr & 0x8000U) != 0) {
        func_800AD698(D_800154CC, dicr);
        for (ch = 0; ch < 7; ch++) {
            unsigned int val = *(unsigned int *)(D_800D4EE8 + ch * 0x10);
            func_800AD698(D_800154E8, ch, val);
        }
    }
}

static dma_cb_t D_800BB144(int ch, dma_cb_t cb)
{
    dma_cb_t prev = D_800D4EC8[ch];

    if (cb == prev) {
        return prev;
    }

    if (cb != 0) {
        D_800D4EC8[ch] = cb;
        *D_800D4EC4 = (*D_800D4EC4 & 0x00FFFFFFU) | (1U << (ch + 16)) | 0x00800000U;
    } else {
        D_800D4EC8[ch] = 0;
        *D_800D4EC4 = ((*D_800D4EC4 & 0x00FFFFFFU) | 0x00800000U) & ~(1U << (ch + 16));
    }

    return prev;
}
