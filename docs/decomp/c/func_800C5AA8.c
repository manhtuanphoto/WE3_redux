/*
 * func_800C5AA8 (and trailing unlabeled BIOS patch/helper fragments)
 *
 * Purpose:
 *   func_800C5AA8 is a PS1 BIOS B0:4C tail-call wrapper (StopCard).
 *   The trailing unlabeled instructions in the same assembly unit (including
 *   D_800C5B28 and D_800C5B3C) are low-level pad/memory-card BIOS patch
 *   routines copied into BIOS RAM by func_800C5B50:
 *     - 0x800C5AB4..0x800C5ADC: modifies *(u16*)(base + 0x0A) |= val | 0x12
 *       and spins for 0x28 (40) iterations before returning.
 *     - 0x800C5AE0..0x800C5B24: polls hardware status registers relative to
 *       $v1 (when $v1 == 0x1F800000: +0x1074 I_MASK bit 7 and +0x1044 JOY_STAT
 *       bit 7), then tail-jumps to *(u32*)0x0000DFFC if bit 7 was set.
 *     - D_800C5B28: jumps to D_A000DFAC.
 *     - D_800C5B3C: calls D_A000DF80 via jalr.
 *
 * Arguments:
 *   func_800C5AA8: None / forwarded to PS1 BIOS B0:4C
 *
 * Return value:
 *   Forwarded from PS1 BIOS B0:4C
 *
 * Note:
 *   The trailing unlabeled fragments are not standalone C-ABI functions; they
 *   expect register state ($v0, $v1, $ra) established by the patched BIOS
 *   handler and are copied verbatim as machine code by func_800C5B50.
 */

extern void D_A000DFAC(void);
extern void D_A000DF80(void);

/* PS1 BIOS B0:4C (StopCard) */
void func_800C5AA8(void)
{
    typedef void (*bios_b0_fn)(void);
    /* UNCLEAR: BIOS tail-jump to 0x000000B0 with $t1 = 0x4C; cannot express $t1 in standard C */
    ((bios_b0_fn)0x000000B0)();
}

/*
 * Unlabeled BIOS patch fragment 1 (0x800C5AB4..0x800C5ADC):
 *   *(u16 *)(v1 + 0x0A) = *(u16 *)(v1 + 0x0A) | v0 | 0x12;
 *   for (t0 = 0x28; t0 != 0; t0--) {}
 *
 * Unlabeled BIOS patch fragment 2 (0x800C5AE0..0x800C5B24):
 *   if ((*(volatile u32 *)(v1 + 0x1074) & 0x80) != 0) {
 *       while ((*(volatile u32 *)(v1 + 0x1044) & 0x80) != 0) {}
 *       ((void (*)(void))(*(volatile u32 *)0x0000DFFC))();
 *   }
 *
 * D_800C5B28 (copied by func_800C5B50):
 *   jumps to D_A000DFAC.
 *
 * D_800C5B3C:
 *   calls D_A000DF80.
 */
