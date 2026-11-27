/*
 * func_800C0B98 (and unlabelled adjacent function func_800C0BC8)
 *
 * Purpose:
 *   func_800C0B98:
 *     Validates an 8-bit count/index in range 1..24 (0x01..0x18). If valid,
 *     stores (u8)val to D_800FB578 and returns (s8)val; otherwise returns -1.
 *   func_800C0BC8 (unlabelled at 0x800C0BC8 in the same assembly file):
 *     PsyQ SsVabOpenHead wrapper: calls func_800C0C58(vabData, vabId, 0, 0)
 *     and returns the sign-extended 16-bit VAB slot ID (or -1 on failure).
 *
 * Arguments:
 *   func_800C0B98:
 *     val (int): byte value tested against range 1..24 (PS1 SPU 24 voices)
 *   func_800C0BC8:
 *     vabData (u8 *): pointer to VAB header data in main RAM
 *     vabId   (s16):  requested VAB slot (0..15) or -1 to auto-allocate
 *
 * Return value:
 *   func_800C0B98: (s8)val if 1 <= (u8)val <= 24, else -1
 *   func_800C0BC8: (s16) allocated VAB slot ID or -1
 *
 * Global fields written:
 *   D_800FB578 (u8): validated byte value
 */

extern unsigned char D_800FB578;

extern int func_800C0C58(unsigned char *vabData, signed short vabId, signed short arg2, unsigned int spuAddr);

int func_800C0B98(int val)
{
    if (((unsigned int)(val - 1) & 0xFFu) < 0x18u) {
        D_800FB578 = (unsigned char)val;
        return (signed char)val;
    }

    return -1;
}

/* Unlabelled function at 0x800C0BC8 in the same assembly file */
signed short func_800C0BC8(unsigned char *vabData, signed short vabId)
{
    return (signed short)func_800C0C58(vabData, vabId, 0, 0u);
}
