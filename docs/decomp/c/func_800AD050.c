/*
 * func_800AD050
 *
 * Purpose:
 *   If (index & 0xFFFF) < 3, clears the 16-bit field at offset +0x0 of the
 *   corresponding 16-byte entry in D_800CEA48 and returns 1; otherwise returns 0.
 *
 * Arguments:
 *   index (int): Entry index (low 16 bits checked against 3)
 *
 * Return value:
 *   int: 1 if index < 3, 0 otherwise
 *
 * Globals read/written:
 *   D_800CEA48 (pointer to array of 16-byte entries, +0x0 u16 cleared)
 */

extern unsigned char *D_800CEA48;

int func_800AD050(int index)
{
    unsigned int idx = (unsigned int)index & 0xFFFFU;

    if (idx >= 3U) {
        return 0;
    }

    /* UNCLEAR: struct layout of 16-byte entries at D_800CEA48 */
    *(unsigned short *)(D_800CEA48 + (idx << 4)) = 0;
    return 1;
}
