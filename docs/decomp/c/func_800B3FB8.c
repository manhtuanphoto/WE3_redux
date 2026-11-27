/*
 * func_800B3FB8
 *
 * Purpose:
 *   Builds a 24-byte (0x18) PS1 GPU packet combining GP0(0xE1) Draw Mode
 *   (texture page / semi-transparency / depth) and a 0x64 SPRT primitive from
 *   a sprite descriptor struct, and links it into the Ordering Table.
 *
 * Arguments:
 *   sprite: (unsigned char*) sprite descriptor struct:
 *             +0x00 (s32) flags (< 0 skips; bits 6, 24..25, 28..30 -> GPU cmd)
 *             +0x04 (u16) X position, +0x06 (u16) Y position
 *             +0x08 (u16) width (0 skips), +0x0A (u16) height (0 skips)
 *             +0x0C (u16) texture page (lower 5 bits)
 *             +0x0E (u8)  U, +0x0F (u8) V
 *             +0x10 (s16) CLUT X, +0x12 (s16) CLUT Y
 *             +0x14..0x16 (u8) R, G, B color
 *   otInfo: (unsigned char*) OT descriptor (+0x04: u32* otBase, +0x08: u32 otOffset)
 *   otZ:    (unsigned short) Z/OT index
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   D_800EF8A8: (u8*) current GPU primitive buffer pointer (advanced by 0x18) (R/W)
 *   D_800F1070: (u16) global screen X offset (R)
 *   D_800F1072: (u16) global screen Y offset (R)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char *D_800EF8A8;
extern unsigned short D_800F1070;
extern unsigned short D_800F1072;

void func_800B3FB8(unsigned char *sprite, unsigned char *otInfo, unsigned short otZ) {
    int flags = SW(sprite, 0x00);
    unsigned char *prim;
    unsigned int *otEntry;
    unsigned int x, y;
    unsigned int clut;

    if (flags < 0) {
        return;
    }
    if (H(sprite, 0x08) == 0 || H(sprite, 0x0A) == 0) {
        return;
    }

    prim = D_800EF8A8;

    /* GP0(0xE1) Draw Mode command word (dither bit 9 always set) */
    W(prim, 0x04) = 0xE1000200U
                  | (((unsigned int)flags >> 17) & 0x180U)
                  | (((unsigned int)flags >> 23) & 0x060U)
                  | (H(sprite, 0x0C) & 0x01FU);

    /* Screen XY with global offset */
    x = (H(sprite, 0x04) + D_800F1070) & 0xFFFFU;
    y = (H(sprite, 0x06) + D_800F1072) & 0xFFFFU;
    W(prim, 0x0C) = x | (y << 16);

    /* GP0(0x64) SPRT command + RGB */
    W(prim, 0x08) = 0x64000000U
                  | (((unsigned int)flags >> 5) & 0x02000000U)
                  | (((unsigned int)flags << 18) & 0x01000000U)
                  | ((unsigned int)B(sprite, 0x16) << 16)
                  | ((unsigned int)B(sprite, 0x15) << 8)
                  | (unsigned int)B(sprite, 0x14);

    /* UV + CLUT */
    clut = ((unsigned int)S(sprite, 0x12) << 22)
         | (((unsigned int)S(sprite, 0x10) << 12) & 0x003F0000U);
    W(prim, 0x10) = (unsigned int)B(sprite, 0x0E)
                  | ((unsigned int)B(sprite, 0x0F) << 8)
                  | clut;

    /* Width / Height */
    W(prim, 0x14) = (unsigned int)H(sprite, 0x08)
                  | ((unsigned int)H(sprite, 0x0A) << 16);

    /* Link 5-word packet into Ordering Table */
    otEntry = (unsigned int *)W(otInfo, 0x04) + ((unsigned int)otZ - W(otInfo, 0x08));
    W(prim, 0x00) = *otEntry + 0x05000000U;
    *otEntry = (unsigned int)prim & 0x00FFFFFFU;

    D_800EF8A8 = prim + 0x18;
}
