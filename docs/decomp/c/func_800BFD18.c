/*
 * func_800BFD18 - Clear current sound/channel slot state and update 32-bit bitmasks
 *
 * Purpose:
 *   Reads the active slot index from D_8010A3E2, clears three fields in the
 *   54-byte (0x36) struct array at D_800E78D8 (+0x00 u16, +0x04 u16, +0x1D u8 at
 *   D_800E78F5), sets the slot's bit in the 32-bit mask split across
 *   D_8010CE6C (bits 0..15) / D_8010CE6E (bits 16..31), and clears all bits
 *   present in those masks from D_800E75F8 / D_800E75FA.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Globals read & written:
 *   D_8010A3E2 (u16, R): active slot index
 *   D_800E78D8 (u8[], W): 54-byte-stride slot table (+0x00 u16, +0x04 D_800E78DC u16, +0x1D D_800E78F5 u8)
 *   D_8010CE6C / D_8010CE6E (u16, R/W): low/high 16-bit slot flag masks
 *   D_800E75F8 / D_800E75FA (u16, R/W): low/high 16-bit complementary masks
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define H(p, o) (*(unsigned short *)((p) + (o)))

extern unsigned short D_8010A3E2;
extern unsigned char D_800E78D8[];
extern unsigned char D_800E78DC[];
extern unsigned char D_800E78F5[];
extern unsigned short D_8010CE6C;
extern unsigned short D_8010CE6E;
extern unsigned short D_800E75F8;
extern unsigned short D_800E75FA;

void func_800BFD18(void)
{
    unsigned int slot = (unsigned int)D_8010A3E2;
    unsigned int mask_lo;
    unsigned int mask_hi;
    unsigned int offset;
    unsigned short new_lo;
    unsigned short new_hi;

    if (slot < 0x10u) {
        mask_lo = 1u << slot;
        mask_hi = 0u;
    } else {
        mask_lo = 0u;
        mask_hi = 1u << (slot - 0x10u);
    }

    offset = slot * 54u; /* 0x36 bytes per entry */
    B(D_800E78F5, offset) = 0;
    H(D_800E78DC, offset) = 0;
    H(D_800E78D8, offset) = 0;

    new_lo = (unsigned short)(D_8010CE6C | mask_lo);
    D_8010CE6C = new_lo;
    D_800E75F8 = (unsigned short)(D_800E75F8 & (unsigned short)(~new_lo));

    new_hi = (unsigned short)(D_8010CE6E | mask_hi);
    D_8010CE6E = new_hi;
    D_800E75FA = (unsigned short)(D_800E75FA & (unsigned short)(~new_hi));
}
