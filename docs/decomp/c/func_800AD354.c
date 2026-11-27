/*
 * func_800AD354
 *
 * Purpose:
 *   Checks whether bit 0 is set in both +0x4 and +0x0 of the 32-bit word pair
 *   pointed to by D_800CEA6C (likely PS1 I_MASK at +0x4 and I_STAT at +0x0,
 *   testing if VBlank IRQ is enabled and pending).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   int: 1 if ((D_800CEA6C[1] & 1) && (D_800CEA6C[0] & 1)), else 0
 *
 * Globals read:
 *   D_800CEA6C (volatile unsigned int *): Base pointer to status/mask registers
 */

extern volatile unsigned int *D_800CEA6C;

int func_800AD354(void)
{
    volatile unsigned int *regs = D_800CEA6C;

    /* UNCLEAR: exact identity of D_800CEA6C (likely 0x1F801070 I_STAT/I_MASK) */
    if ((regs[1] & 1U) == 0U) {
        return 0;
    }
    if ((regs[0] & 1U) != 0U) {
        return 1;
    }
    return 0;
}
