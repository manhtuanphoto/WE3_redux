/*
 * func_800C18DC
 *
 * Purpose:
 *   Low-level SPU/DMA transfer control dispatcher (varargs: cmd, ...):
 *     cmd 0: select mode 1, wait for status match on *D_800D558C+0x1A6, set 0x30 bits in +0x1AA
 *     cmd 1: select mode 0, wait for status match on *D_800D558C+0x1A6, set 0x20 in +0x1AA
 *     cmd 2: set transfer address/offset (arg1 >> D_800D55B4) in D_800D55A4 and +0x1A6
 *     cmd 3: wait for +0x1AA status bits, configure DMA (func_800C1DE8/func_800C1DC0) and start
 *            transfer of arg2 bytes from/to arg1 using 64-byte blocks
 *
 * Arguments:
 *   cmd:  (int) command selector (0..3)
 *   arg1: (unsigned int, vararg) address/offset depending on cmd (used by cmd 2 and 3)
 *   arg2: (unsigned int, vararg) byte length (used by cmd 3)
 *
 * Return value:
 *   (int) 0 on success (or unknown cmd), -2 on hardware polling timeout (0xF00 tries)
 *
 * Reads / Writes:
 *   D_800D558C (u8*): hardware register base (+0x1A6 u16, +0x1AA u16) (R/W)
 *   D_800D5590 / D_800D5594 / D_800D5598 (u32*): DMA MADR/BCR/CHCR register pointers (W)
 *   D_800D55A4 (u16): cached address register value (R/W)
 *   D_800D55B4 (u32): address shift count (R)
 *   D_800D55DC (u32): transfer direction/mode flag (0 or 1) (R/W)
 *   D_800D55E0 (u32): DMA base address (W/R)
 *   D_800D55E4 (u32): DMA block count = ceil(arg2 / 64) (W/R)
 *
 * Magic numbers:
 *   0xF00 (3840): polling loop timeout count.
 *   0x01000200 / 0x01000201: DMA channel control words.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char *D_800D558C;
extern unsigned int *D_800D5590;
extern unsigned int *D_800D5594;
extern unsigned int *D_800D5598;
extern unsigned short D_800D55A4;
extern unsigned int D_800D55B4;
extern unsigned int D_800D55DC;
extern unsigned int D_800D55E0;
extern unsigned int D_800D55E4;
extern void func_800C1DE8(void);
extern void func_800C1DC0(void);

int func_800C18DC(int cmd, unsigned int arg1, unsigned int arg2, unsigned int arg3) {
    unsigned int timeout;
    unsigned char *regs;
    unsigned short target;
    unsigned int expected;
    unsigned int chcr;

    (void)arg3;

    switch (cmd) {
    case 0:
        regs = D_800D558C;
        target = D_800D55A4;
        D_800D55DC = 1;
        if (H(regs, 0x1A6) != target) {
            timeout = 1;
            while (1) {
                if (timeout >= 0xF01u) {
                    return -2;
                }
                if (H(regs, 0x1A6) == target) {
                    break;
                }
                timeout++;
            }
        }
        regs = D_800D558C;
        H(regs, 0x1AA) |= 0x30u;
        return 0;

    case 1:
        regs = D_800D558C;
        target = D_800D55A4;
        D_800D55DC = 0;
        if (H(regs, 0x1A6) != target) {
            timeout = 1;
            while (1) {
                if (timeout >= 0xF01u) {
                    return -2;
                }
                if (H(regs, 0x1A6) == target) {
                    break;
                }
                timeout++;
            }
        }
        regs = D_800D558C;
        H(regs, 0x1AA) = (unsigned short)((H(regs, 0x1AA) & 0xFFCFu) | 0x20u);
        return 0;

    case 2:
        {
            unsigned short val = (unsigned short)(arg1 >> D_800D55B4);
            D_800D55A4 = val;
            H(D_800D558C, 0x1A6) = val;
        }
        return 0;

    case 3:
        expected = (D_800D55DC == 1u) ? 0x30u : 0x20u;
        regs = D_800D558C;
        if ((H(regs, 0x1AA) & 0x30u) != expected) {
            timeout = 1;
            while (1) {
                if (timeout >= 0xF01u) {
                    return -2;
                }
                if ((H(regs, 0x1AA) & 0x30u) == expected) {
                    break;
                }
                timeout++;
            }
        }

        if (D_800D55DC == 1u) {
            func_800C1DE8();
        } else {
            func_800C1DC0();
        }

        D_800D55E0 = arg1;
        D_800D55E4 = (arg2 >> 6) + (((arg2 & 0x3Fu) != 0u) ? 1u : 0u);
        *D_800D5590 = D_800D55E0;
        *D_800D5594 = (D_800D55E4 << 16) | 0x10u;
        chcr = (D_800D55DC == 1u) ? 0x01000200u : 0x01000201u;
        *D_800D5598 = chcr;
        return 0;

    default:
        return 0;
    }
}
