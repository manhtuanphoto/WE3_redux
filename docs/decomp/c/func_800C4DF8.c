/*
 * func_800C4DF8
 *
 * Purpose:
 *   Reads the 16-bit unsigned field at offset +0xC of the 16-byte entry
 *   D_800D558C[index], writes it to *outVal, and returns it.
 *
 *   Note: The assembly file also contains an unlabelled trailing function at
 *   +0x20 (immediately after `jr $ra`) that swaps/stores `arg0` into D_8010A408
 *   and returns the previous value of D_8010A408.
 *
 * Arguments:
 *   index (int): Entry index into 16-byte struct array D_800D558C
 *   outVal (unsigned short *): Output pointer receiving the 16-bit value
 *
 * Return value:
 *   unsigned short: The 16-bit value read from offset +0xC
 *
 * Globals read:
 *   D_800D558C (unsigned char *): Base pointer to 16-byte entry array
 */

extern unsigned char *D_800D558C;

unsigned short func_800C4DF8(int index, unsigned short *outVal)
{
    /* UNCLEAR: struct type of 16-byte entries at D_800D558C */
    unsigned char *entry = D_800D558C + (index << 4);
    unsigned short val = *(unsigned short *)(entry + 0xC);

    *outVal = val;
    return val;
}

/*
 * Unreachable/unlabelled adjacent function at +0x20 in func_800C4DF8.s:
 *
 * extern void *D_8010A408;
 * void *func_800C4E18(void *ptr)
 * {
 *     void *prev = D_8010A408;
 *     D_8010A408 = ptr;
 *     return prev;
 * }
 */
