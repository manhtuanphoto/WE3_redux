/*
 * func_800C0BF8 (plus an identical unlabeled adjacent function in func_800C0BF8.s)
 *
 * Purpose:
 *   Wrapper around func_800C0C58 that opens/parses a PS1 VAB soundbank header
 *   using an explicit SPU RAM address (passes arg2 = 1 to func_800C0C58) and
 *   returns the allocated VAB slot ID as a signed 16-bit integer (or -1 on error).
 *
 * Arguments:
 *   vabData (unsigned char *): pointer to VAB header data in main RAM ("pBAV")
 *   vabId   (signed short):    requested VAB slot (0..15) or -1 to auto-allocate
 *   spuAddr (unsigned int):    explicit SPU RAM address for VAG data
 *
 * Return value:
 *   signed short: allocated VAB slot ID (0..15) on success, or -1 on error.
 */

extern int func_800C0C58(unsigned char *vabData, signed short vabId, signed short arg2, unsigned int spuAddr);

signed short func_800C0BF8(unsigned char *vabData, signed short vabId, unsigned int spuAddr)
{
    return (signed short)func_800C0C58(vabData, vabId, 1, spuAddr);
}

/* UNCLEAR: unlabeled second identical function inside func_800C0BF8.s (at 0x800C0C28) */
signed short func_800C0C28(unsigned char *vabData, signed short vabId, unsigned int spuAddr)
{
    return (signed short)func_800C0C58(vabData, vabId, 1, spuAddr);
}
