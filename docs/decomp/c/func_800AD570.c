/*
 * func_800AD570
 *
 * Purpose:
 *   Not a standalone callable function: this is the 16-byte machine-code patch
 *   snippet [func_800AD570 .. D_800AD580) copied by func_800AD4E4 into the PS1
 *   BIOS B0:0x5B handler at (handler_base + 0x3D8) and (handler_base + 0x4E0).
 *
 * Assembly contents:
 *   0x800AD570: and $v0, $v0, $s5  (0x00551024)
 *   0x800AD574..0x800AD57C:
 *     UNCLEAR: 3 words (12 bytes) between 0x800AD574 and D_800AD580 were
 *     omitted from asm_compact (likely 0x00000000 nop padding). No `jr $ra`
 *     is present because the snippet executes inline inside the BIOS handler.
 *
 * Referenced by:
 *   func_800AD4E4 (copies words from func_800AD570 until D_800AD580)
 */

const unsigned int func_800AD570[] = {
    0x00551024U, /* and $v0, $v0, $s5 */
    /* UNCLEAR: remaining 3 words up to D_800AD580 omitted by asm_compact (likely 0x00000000 nop) */
    0x00000000U,
    0x00000000U,
    0x00000000U
};
