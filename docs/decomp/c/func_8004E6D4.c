/*
 * func_8004E6D4
 *
 * Purpose:
 *   Looks up a 6-byte descriptor entry in a per-ID ROM/RAM table (indexed by
 *   an 8-bit primary ID `id` in [6..119] and a sub-index `sub` that is clamped
 *   or offset per ID). Likely animation/action descriptor lookup.
 *
 * Arguments:
 *   id  (u32, masked to u8): primary table/action/animation ID
 *   sub (u32, masked to u8): variant/sub-index within that ID's table
 *
 * Return value:
 *   Pointer (unsigned char *) to the 6-byte entry (or default entry D_800C6DA8).
 *
 * Scratchpad / Actor fields read or written:
 *   None.
 *
 * Magic numbers / units:
 *   Table stride = 6 bytes per sub-entry.
 *   Valid switch range: id in 0x06..0x77 (6..119).
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned char D_800C6DA8[];
extern unsigned char D_800C6DB8[];
extern unsigned char D_800C6DC0[];
extern unsigned char D_800C6DE4[];
extern unsigned char D_800C6E34[];
extern unsigned char D_800C6E54[];
extern unsigned char D_800C6E90[];
extern unsigned char D_800C6EA8[];
extern unsigned char D_800C6EB0[];
extern unsigned char D_800C6EB8[];
extern unsigned char D_800C6EC0[];
extern unsigned char D_800C6F58[];
extern unsigned char D_800C7018[];
extern unsigned char D_800C703C[];
extern unsigned char D_800C7074[];
extern unsigned char D_800C707C[];
extern unsigned char D_800C708C[];
extern unsigned char D_800C7094[];
extern unsigned char D_800C709C[];
extern unsigned char D_800C70A4[];
extern unsigned char D_800C70AC[];
extern unsigned char D_800C70C0[];
extern unsigned char D_800C70C8[];
extern unsigned char D_800C70D0[];
extern unsigned char D_800C7118[];
extern unsigned char D_800C71B4[];
extern unsigned char D_800C71BC[];
extern unsigned char D_800C71C4[];
extern unsigned char D_800C71CC[];
extern unsigned char D_800C71D4[];
extern unsigned char D_800C71DC[];
extern unsigned char D_800C71E4[];
extern unsigned char D_800C7210[];
extern unsigned char D_800C7278[];
extern unsigned char D_800C72F0[];
extern unsigned char D_800C73C8[];
extern unsigned char D_800C7458[];
extern unsigned char D_800C7500[];
extern unsigned char D_800C7578[];
extern unsigned char D_800C75D4[];
extern unsigned char D_800C76AC[];
extern unsigned char D_800C7744[];
extern unsigned char D_800C774C[];
extern unsigned char D_800C779C[];
extern unsigned char D_800C782C[];
extern unsigned char D_800C7964[];
extern unsigned char D_800C79DC[];
extern unsigned char D_800C7A68[];
extern unsigned char D_800C7A70[];
extern unsigned char D_800C7AE4[];
extern unsigned char D_800C7B74[];

unsigned char *func_8004E6D4(unsigned int id, unsigned int sub)
{
    unsigned int idx;

    id &= 0xFF;
    idx = id - 6;
    if (idx >= 0x72) {
        return D_800C6DA8;
    }

    switch (id) {
    case 0x06:
        return D_800C6E54 + (sub & 0xFF) * 6;
    case 0x08:
        if ((sub & 0xFF) >= 4) {
            sub = 3;
        }
        return D_800C6E90 + (sub & 0xFF) * 6;
    case 0x0A:
        if ((sub & 0xFF) >= 3) {
            sub = 2;
        }
        return D_800C6DE4 + (sub & 0xFF) * 6;
    case 0x0B:
        if ((sub & 0xFF) >= 6) {
            sub = 5;
        }
        return D_800C7018 + (sub & 0xFF) * 6;
    case 0x0C:
        if ((sub & 0xFF) >= 5) {
            sub = 4;
        }
        return D_800C6E34 + (sub & 0xFF) * 6;
    case 0x0F:
        return D_800C70A4;
    case 0x10:
        return D_800C6EA8;
    case 0x12:
        return D_800C6EB8;
    case 0x13:
        if ((sub & 0xFF) >= 3) {
            sub = 2;
        }
        return D_800C70AC + (sub & 0xFF) * 6;
    case 0x18:
        return D_800C6F58 + (sub & 0xFF) * 6;
    case 0x1C:
        return D_800C71C4;
    case 0x1D:
        if ((sub & 0xFF) >= 12) {
            sub = 11;
        }
        return D_800C6EC0 + (sub & 0xFF) * 6;
    case 0x1E:
        if ((sub & 0xFF) >= 12) {
            sub = 11;
        }
        return D_800C70D0 + (sub & 0xFF) * 6;
    case 0x20:
        if ((sub & 0xFF) >= 9) {
            sub = 8;
        }
        return D_800C703C + (sub & 0xFF) * 6;
    case 0x23:
        return D_800C71CC;
    case 0x24:
        return D_800C7074;
    case 0x28:
        return D_800C6EB0;
    case 0x2B:
        return D_800C707C;
    case 0x2C:
        return D_800C708C;
    case 0x2D:
        return D_800C7094;
    case 0x2E:
        return D_800C709C;
    case 0x31:
        return D_800C70C0;
    case 0x32:
        return D_800C70C8;
    case 0x4C:
        return D_800C7118 + (sub & 0xFF) * 6;
    case 0x4D:
        return D_800C71B4;
    case 0x53:
        return D_800C71BC;
    case 0x54:
        return D_800C71D4;
    case 0x56:
        return D_800C6DB8;
    case 0x58:
        if ((sub & 0xFF) >= 6) {
            sub = 5;
        }
        return D_800C6DC0 + (sub & 0xFF) * 6;
    case 0x5B:
        return D_800C71DC;
    case 0x5C:
        return D_800C71E4 + (sub & 0xFF) * 6;
    case 0x5D:
        return D_800C7210 + (sub & 0xFF) * 6;
    case 0x5F:
        return D_800C7278 + (sub & 0xFF) * 6;
    case 0x60:
        return D_800C72F0 + (sub & 0xFF) * 6;
    case 0x61:
        return D_800C73C8 + (sub & 0xFF) * 6;
    case 0x62:
        return D_800C7458 + (sub & 0xFF) * 6;
    case 0x63:
        if ((sub & 0xFF) < 10) {
            sub = 0;
        } else {
            sub -= 9;
        }
        return D_800C7578 + (sub & 0xFF) * 6;
    case 0x67:
        return D_800C75D4 + (sub & 0xFF) * 6;
    case 0x68:
        if ((sub & 0xFF) >= 25) {
            sub = 24;
        }
        return D_800C76AC + (sub & 0xFF) * 6;
    case 0x69:
        return D_800C782C + (sub & 0xFF) * 6;
    case 0x6A:
        return D_800C7744;
    case 0x6B:
        if ((sub & 0xFF) >= 22) {
            sub = 12;
        } else if ((sub & 0xFF) < 9) {
            sub = 0;
        } else {
            sub -= 9;
        }
        return D_800C774C + (sub & 0xFF) * 6;
    case 0x6D:
        return D_800C7A70 + (sub & 0xFF) * 6;
    case 0x6E:
        return D_800C7AE4 + (sub & 0xFF) * 6;
    case 0x6F:
        return D_800C779C + (sub & 0xFF) * 6;
    case 0x70:
        if ((sub & 0xFF) >= 20) {
            sub = 19;
        }
        return D_800C7964 + (sub & 0xFF) * 6;
    case 0x71:
        return D_800C7500 + (sub & 0xFF) * 6;
    case 0x72:
        return D_800C79DC + (sub & 0xFF) * 6;
    case 0x73:
        if ((sub & 0xFF) < 13) {
            return D_800C7A68;
        }
        return D_800C7B74;
    case 0x77:
        return D_800C7B74;
    default:
        return D_800C6DA8;
    }
}
