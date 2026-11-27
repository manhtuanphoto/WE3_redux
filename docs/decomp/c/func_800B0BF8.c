/*
 * func_800B0BF8
 *
 * Purpose:
 *   Probes PS1 GPU type/revision via GP1(0x10) Get GPU Info (arg 7 = GPU version)
 *   and configures GPU draw/display mode accordingly (PsyQ GetGraphType-like).
 *   - Writes 0x10000007 to *D_800CEB9C (GPUSTAT/GP1).
 *   - If (*D_800CEB98 & 0x00FFFFFF) == 2: if (flags & 0x8), writes 0x09000001
 *     to *D_800CEB9C and returns 2; otherwise returns 1.
 *   - Otherwise: writes ((*D_800CEB9C & 0x3FFF) | 0xE1001000) to *D_800CEB98
 *     (GP0 Draw Mode), performs a dummy read of *D_800CEB98, and returns 0.
 *
 * Arguments:
 *   flags (unsigned int) - option flags (bit 3 = 0x8 checked when GPU type == 2)
 *
 * Return value:
 *   (int) 0, 1, or 2 depending on GPU response and flags.
 *
 * Global fields read/written:
 *   D_800CEB98 (volatile u32 *) GP0 / GPUREAD register pointer (0x1F801810)
 *   D_800CEB9C (volatile u32 *) GP1 / GPUSTAT register pointer (0x1F801814)
 */

extern volatile unsigned int *D_800CEB98;
extern volatile unsigned int *D_800CEB9C;

int func_800B0BF8(unsigned int flags)
{
    volatile unsigned int dummy;

    *D_800CEB9C = 0x10000007U;

    if ((*D_800CEB98 & 0x00FFFFFFU) == 2U) {
        if ((flags & 0x8U) != 0U) {
            *D_800CEB9C = 0x09000001U;
            return 2;
        }
        return 1;
    }

    *D_800CEB98 = (*D_800CEB9C & 0x3FFFU) | 0xE1001000U;
    dummy = *D_800CEB98;
    (void)dummy;
    return 0;
}
