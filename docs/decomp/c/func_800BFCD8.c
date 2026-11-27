/*
 * func_800BFCD8
 *
 * Purpose:
 *   Clears three state fields of voice slot `voice & 0xFF` in the 54-byte
 *   (0x36) voice/channel struct table at D_800E78D8: byte +0x1D (D_800E78F5),
 *   halfword +0x00 (D_800E78D8), and halfword +0x04 (D_800E78DC).
 *
 * Arguments:
 *   voice (unsigned int): voice slot index (masked to 0xFF)
 *
 * Return value:
 *   None (void)
 *
 * Global fields written:
 *   D_800E78D8 (u8[]): 54-byte-stride voice slot table (+0x00 u16 = 0)
 *   D_800E78DC (u8[]): alias at D_800E78D8 + 0x04 (+0x04 u16 = 0)
 *   D_800E78F5 (u8[]): alias at D_800E78D8 + 0x1D (+0x1D u8 = 0)
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define H(p, o) (*(unsigned short *)((p) + (o)))

extern unsigned char D_800E78D8[];
extern unsigned char D_800E78DC[];
extern unsigned char D_800E78F5[];

void func_800BFCD8(unsigned int voice)
{
    unsigned int offset = (voice & 0xFFU) * 54U; /* 0x36 bytes per entry */

    B(D_800E78F5, offset) = 0;
    H(D_800E78D8, offset) = 0;
    H(D_800E78DC, offset) = 0;
}
