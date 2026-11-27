/*
 * func_800ACF18
 *
 * Purpose:
 *   Configures one of the three PS1 hardware root counters (0..2) via register
 *   array `*D_800CEA48` (0x1F801100 base, 16-byte stride: +0x04 mode, +0x08
 *   target), implementing PsyQ `SetRCnt(spec, target, mode)`.
 *   - Returns 0 if `(spec & 0xFFFF) >= 3`.
 *   - Clears mode to 0, writes target, builds hardware mode bits from `mode`
 *     (bit 0x10 -> clock source 0x49 vs 0x48 on cnt 0..1; bit 0x1 == 0 ->
 *     0x100 clock source on cnt 0..1 or 0x248 SysClk/8 on cnt 2; bit 0x1000
 *     -> 0x10 target IRQ enable), writes mode, and returns 1.
 *
 * Arguments:
 *   spec   (unsigned int)   - root counter descriptor (low 16 bits: 0..2)
 *   target (unsigned short) - 16-bit target count written to +0x08
 *   mode   (unsigned int)   - PsyQ RCnt mode flags (0x1, 0x10, 0x1000)
 *
 * Return value:
 *   (int) 1 on success, 0 if `(spec & 0xFFFF) >= 3`.
 *
 * Global fields read/written:
 *   D_800CEA48 (volatile u8 *) base pointer to PS1 root counter registers (0x1F801100)
 */

#define H(p, o) (*(volatile unsigned short *)((p) + (o)))

extern volatile unsigned char *D_800CEA48;

int func_800ACF18(unsigned int spec, unsigned short target, unsigned int mode)
{
    unsigned int idx = spec & 0xFFFFU;
    volatile unsigned char *reg;
    unsigned int hwMode = 0x48U;

    if ((int)idx >= 3) {
        return 0;
    }

    reg = D_800CEA48 + (idx << 4);
    H(reg, 0x04) = 0;
    H(reg, 0x08) = target;

    if (idx < 2U) {
        if ((mode & 0x10U) != 0U) {
            hwMode = 0x49U;
        }
        if ((mode & 0x1U) == 0U) {
            hwMode |= 0x100U;
        }
    } else if (idx == 2U) {
        if ((mode & 0x1U) == 0U) {
            hwMode = 0x248U;
        }
    }

    if ((mode & 0x1000U) != 0U) {
        hwMode |= 0x10U;
    }

    H(D_800CEA48 + (idx << 4), 0x04) = (unsigned short)hwMode;
    return 1;
}
