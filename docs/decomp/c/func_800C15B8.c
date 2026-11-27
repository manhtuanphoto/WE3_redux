/*
 * func_800C15B8 (plus adjacent routines D_800C1778 and D_800C182C in the .s)
 *
 * Purpose:
 *   PsyQ SPU transfer routines (operating on SPU register base D_800D558C,
 *   offsets +0x1A6 transfer addr, +0x1A8 FIFO, +0x1AA control, +0x1AE status):
 *   - func_800C15B8: PIO-writes `size` bytes from `data` to SPU RAM in up to
 *     64-byte chunks, polling SPU status bits 0x400 (busy) and 0x7FF (address/
 *     handshake) with timeout 0xF00 (3840 iterations) and reporting errors via
 *     func_800AD698.
 *   - D_800C1778: Stops SPU transfer mode (clears bits 0x30 in +0x1AA), waits
 *     for completion, and invokes callback D_800D55C4 or func_800B6E78.
 *   - D_800C182C: Starts an SPU DMA read transfer from SPU address `spu_addr`
 *     into main RAM `ram_addr` with block control `(blocks << 16) | 0x10`.
 *
 * Arguments:
 *   func_800C15B8(unsigned short *data, int size)
 *   D_800C1778(void)
 *   D_800C182C(unsigned int ram_addr, unsigned short spu_addr, unsigned int blocks)
 *
 * Return value:
 *   void
 *
 * Global fields read/written:
 *   D_800D558C (u8 * -> SPU register base),
 *   D_800D55A4 (u16) SPU transfer address,
 *   D_800D5590 / D_800D5594 / D_800D5598 (u32 *) DMA MADR/BCR/CHCR pointers,
 *   D_800D55C4 (u32) transfer completion callback pointer,
 *   D_800D55DC (u32) DMA active flag,
 *   D_8001551C / D_8001553C / D_80015550 (error format/message strings)
 *
 * Scratchpad fields read/written:
 *   None
 *
 * Magic numbers:
 *   0x40 (64 bytes) - maximum PIO chunk size per SPU FIFO burst
 *   0xF00 (3840) - SPU status polling timeout loop limit (< 0xF01)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char *D_800D558C;
extern unsigned short D_800D55A4;
extern unsigned int *D_800D5590;
extern unsigned int *D_800D5594;
extern unsigned int *D_800D5598;
extern unsigned int D_800D55C4;
extern unsigned int D_800D55DC;
extern char D_8001551C[];
extern char D_8001553C[];
extern char D_80015550[];

extern void func_800C1E10(void);
extern void func_800C1DE8(void);
extern void func_800AD698(const char *fmt, const char *msg);
extern void func_800B6E78(unsigned int event, unsigned int mask);

void func_800C15B8(unsigned short *data, int size)
{
    unsigned int status_low;
    int chunk;
    int i;
    unsigned int timeout;

    H(D_800D558C, 0x1A6) = D_800D55A4;
    status_low = H(D_800D558C, 0x1AE) & 0x7FFU;
    func_800C1E10();

    while (size != 0) {
        chunk = ((unsigned int)size < 0x41U) ? size : 0x40;
        if (chunk > 0) {
            unsigned char *spu = D_800D558C;
            for (i = 0; i < chunk; i += 2) {
                H(spu, 0x1A8) = *data++;
            }
        }

        H(D_800D558C, 0x1AA) = (unsigned short)((H(D_800D558C, 0x1AA) & 0xFFCF) | 0x10);
        func_800C1E10();

        if ((H(D_800D558C, 0x1AE) & 0x400) != 0) {
            timeout = 1;
            while (1) {
                if (timeout >= 0xF01U) {
                    func_800AD698(D_8001551C, D_8001553C);
                    break;
                }
                if ((H(D_800D558C, 0x1AE) & 0x400) == 0) {
                    break;
                }
                timeout++;
            }
        }

        func_800C1E10();
        size -= chunk;
        func_800C1E10();
    }

    H(D_800D558C, 0x1AA) = (unsigned short)(H(D_800D558C, 0x1AA) & 0xFFCF);
    if ((H(D_800D558C, 0x1AE) & 0x7FFU) != status_low) {
        timeout = 1;
        while (1) {
            if (timeout >= 0xF01U) {
                func_800AD698(D_8001551C, D_80015550);
                break;
            }
            if ((H(D_800D558C, 0x1AE) & 0x7FFU) == status_low) {
                break;
            }
            timeout++;
        }
    }
}

/* Adjacent entry at D_800C1778 in func_800C15B8.s */
void D_800C1778(void)
{
    unsigned char *spu;
    unsigned int timeout;

    if (D_800D55DC == 0) {
        func_800C1E10();
    }
    spu = D_800D558C;
    H(spu, 0x1AA) = (unsigned short)(H(spu, 0x1AA) & 0xFFCF);
    if ((H(spu, 0x1AA) & 0x30) != 0) {
        timeout = 1;
        while (timeout < 0xF01U) {
            if ((H(spu, 0x1AA) & 0x30) == 0) {
                break;
            }
            timeout++;
        }
    }
    if (D_800D55C4 != 0) {
        ((void (*)(void))D_800D55C4)();
    } else {
        func_800B6E78(0xF0000009U, 0x20U);
    }
}

/* Adjacent unnamed routine at 0x800C182C in func_800C15B8.s */
void D_800C182C(unsigned int ram_addr, unsigned short spu_addr, unsigned int blocks)
{
    H(D_800D558C, 0x1A6) = spu_addr;
    func_800C1E10();
    H(D_800D558C, 0x1AA) = (unsigned short)(H(D_800D558C, 0x1AA) | 0x30);
    func_800C1E10();
    func_800C1DE8();
    *D_800D5590 = ram_addr;
    *D_800D5594 = (blocks << 16) | 0x10U;
    D_800D55DC = 1;
    *D_800D5598 = 0x01000200U;
}
