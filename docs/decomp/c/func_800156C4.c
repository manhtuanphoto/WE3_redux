/*
 * func_800156C4
 *
 * Purpose:
 *   Program entry point (CRT0 startup), encoded as raw .word directives in
 *   asm_compact. Clears the BSS range [0x800E6AC0, 0x8010CE70), initializes
 *   $sp/$fp from (*(u32 *)0x80015774 | 0x80000000) and $gp to 0x800E6A8C,
 *   computes heap bounds from 0x8010CE70..((*(u32 *)0x80015774) - D_800D4F04),
 *   calls func_800ACE18(heap_start + 4, heap_size) (InitHeap pattern),
 *   calls func_80015780() (main), and executes `break 1` (0x0000004D) on return.
 *
 * Raw instruction notes:
 *   0x3C02800E / 0x24426AC0 : $v0 = 0x800E6AC0 (BSS start)
 *   0x3C038011 / 0x2463CE70 : $v1 = 0x8010CE70 (BSS end / heap base)
 *   0xAC400000..0x1420FFFC  : zero-fill loop over [0x800E6AC0, 0x8010CE70)
 *   0x3C048001 / 0x24845770 + 4 : reads *(u32 *)0x80015774; sets $sp = val | 0x80000000
 *   0x8C634F04              : $v1 = D_800D4F04 (stack size subtracted from stack top)
 *   0xAC3F6AC0 / 0x8FFF6AC0 : preserves $ra across func_800ACE18 via 0x800E6AC0
 *   0x3C1C800E / 0x279C6A8C : sets $gp = 0x800E6A8C
 *   0x0C02B386              : jal func_800ACE18(heap_start + 4, heap_size)
 *   0x0C0055E0              : jal func_80015780()
 *   0x0000004D              : break 1 (followed by 4 x 0x00200000 padding words)
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 */

extern unsigned int D_80015774;
extern unsigned int D_800D4F04;

extern void func_800ACE18(void *heap_start, unsigned int heap_size);
extern void func_80015780(void);

void func_800156C4(void)
{
    volatile unsigned int *p;
    unsigned int stack_top_raw;
    unsigned int stack_size;
    unsigned int heap_base;
    unsigned int heap_size;

    for (p = (volatile unsigned int *)0x800E6AC0U;
         p < (volatile unsigned int *)0x8010CE70U;
         p++) {
        *p = 0U;
    }

    stack_top_raw = D_80015774;
    /* UNCLEAR: C cannot directly assign CPU registers $sp = (stack_top_raw | 0x80000000),
     * $gp = 0x800E6A8C, $fp = $sp, or preserve $ra in 0x800E6AC0 across InitHeap.
     */

    heap_base = (0x8010CE70U << 3) >> 3; /* physical address 0x0010CE70 */
    stack_size = D_800D4F04;
    heap_size = (stack_top_raw - stack_size) - heap_base;
    heap_base |= 0x80000000U;

    func_800ACE18((void *)(heap_base + 4U), heap_size);
    func_80015780();

    /* Assembly executes `break 1` (0x0000004D) if func_80015780 returns. */
}
