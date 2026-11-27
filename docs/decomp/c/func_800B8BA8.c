/*
 * func_800B8BA8 (and adjacent routines at 0x800B8C94 and D_800B8CA0)
 *
 * Purpose:
 *   Decoded from raw `.word` MIPS R3000A instructions in asm_compact:
 *   1) func_800B8BA8: Initiates a CD-ROM DMA transfer (likely PsyQ CD_read / DMA3)
 *      via volatile hardware register pointers in D_800D3C14..D_800D3C58, waits for
 *      bit 6 (0x40) of *D_800D3C14, starts DMA with 0x11400100, and returns 0.
 *   2) func_800B8C94 (unlabeled at +0xEC): Sets D_800D3BF8 = a0.
 *   3) D_800B8CA0 (labeled in .s): CD-ROM interrupt/status dispatcher that polls
 *      func_800B7618() and invokes callbacks D_800D3950 (when bit 2 / 0x4 is set)
 *      and D_800D394C (when bit 1 / 0x2 is set), restoring (*D_800D3C14 & 3).
 *
 * Arguments (func_800B8BA8):
 *   dst_addr (unsigned int): destination RAM address written to *D_800D3C50 (DMA MADR)
 *   block_ctrl (unsigned int): transfer size/count; written as (block_ctrl | 0x00010000) to *D_800D3C54
 *
 * Return value (func_800B8BA8):
 *   0
 *
 * Global hardware-register pointers read/written:
 *   D_800D3C14 (volatile unsigned char *): CD index/status reg (bank 0, polls & 0x40, saves/restores & 3)
 *   D_800D3C20 (volatile unsigned char *): written 0x80
 *   D_800D3C24 (volatile unsigned int *): written 0x1325
 *   D_800D3C48 (volatile unsigned int *): written 0x21020843
 *   D_800D3C4C (volatile unsigned int *): ORed with 0x8000 (DMA DPCR enable)
 *   D_800D3C50 (volatile unsigned int *): DMA MADR (written dst_addr)
 *   D_800D3C54 (volatile unsigned int *): DMA BCR (written block_ctrl | 0x00010000)
 *   D_800D3C58 (volatile unsigned int *): DMA CHCR (written 0x11400100, then read back)
 *   D_800D3BF8 (unsigned int): written by func_800B8C94
 *   D_800D3C2C (unsigned char), D_800D3C2D (unsigned char)
 *   D_800D394C, D_800D3950: callback function pointers
 *   D_800E71C8, D_800E71D0: buffers passed to callbacks
 *
 * Notes:
 *   Trailing words at end of func_800B8BA8.s (0x19017350, 0x00415CB3) appear to be
 *   non-instruction padding/data and are omitted.
 */

extern volatile unsigned char *D_800D3C14;
extern volatile unsigned char *D_800D3C20;
extern volatile unsigned int  *D_800D3C24;
extern volatile unsigned int  *D_800D3C48;
extern volatile unsigned int  *D_800D3C4C;
extern volatile unsigned int  *D_800D3C50;
extern volatile unsigned int  *D_800D3C54;
extern volatile unsigned int  *D_800D3C58;

extern unsigned int D_800D3BF8;
extern unsigned char D_800D3C2C;
extern unsigned char D_800D3C2D;
extern void (*D_800D394C)(unsigned int status, unsigned char *buf);
extern void (*D_800D3950)(unsigned int status, unsigned char *buf);
extern unsigned char D_800E71C8[];
extern unsigned char D_800E71D0[];

extern unsigned int func_800B7618(void);

int func_800B8BA8(unsigned int dst_addr, unsigned int block_ctrl)
{
    volatile unsigned int dummy;

    *D_800D3C14 = 0;
    *D_800D3C20 = 0x80;
    *D_800D3C48 = 0x21020843u;
    *D_800D3C24 = 0x1325u;
    *D_800D3C4C = *D_800D3C4C | 0x8000u;
    *D_800D3C50 = dst_addr;
    *D_800D3C54 = block_ctrl | 0x00010000u;

    while ((*D_800D3C14 & 0x40u) == 0) {
        /* Wait until DRQSTS / data request ready */
    }

    *D_800D3C58 = 0x11400100u;
    dummy = *D_800D3C58;
    (void)dummy;

    return 0;
}

/* Unlabeled subroutine at 0x800B8C94 */
void func_800B8C94(unsigned int a0)
{
    D_800D3BF8 = a0;
}

/* Labeled D_800B8CA0 in func_800B8BA8.s */
void D_800B8CA0(void)
{
    unsigned char saved_bank = (unsigned char)(*D_800D3C14 & 0x03u);
    unsigned int flags;

    for (;;) {
        flags = func_800B7618();
        if (flags == 0) {
            break;
        }

        if ((flags & 0x04u) != 0 && D_800D3950 != 0) {
            D_800D3950(D_800D3C2D, D_800E71D0);
        }

        if ((flags & 0x02u) != 0 && D_800D394C != 0) {
            D_800D394C(D_800D3C2C, D_800E71C8);
        }
    }

    *D_800D3C14 = saved_bank;
}
