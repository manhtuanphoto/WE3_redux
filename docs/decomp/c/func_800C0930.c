/*
 * func_800C0930
 *
 * Purpose:
 *   Iterates over active voice/sound slots (0 .. D_800FB578 - 1). For each slot
 *   whose bit is clear in D_800D4F0C and whose 16-bit ID at D_800E78E8[slot * 0x36]
 *   matches `targetId`, stores the slot index into D_8010A3E2 and calls
 *   func_800BFD18(0).
 *
 * Arguments:
 *   targetId (s16 in int) - 16-bit sound/voice ID to match against D_800E78E8 entries
 *
 * Return value:
 *   None (void)
 *
 * Globals read/written:
 *   D_800FB578 (s8, R)    - slot count upper bound
 *   D_800D4F0C (u32, R)   - bitmask of masked/inactive/locked slots
 *   D_800E78E8 (u8[], R)  - table of 0x36-byte (54-byte) slot records (s16 ID at +0x00)
 *   D_8010A3E2 (u16, W)   - matched slot index passed implicitly to func_800BFD18
 */

#define S(p, o) (*(signed short *)((p) + (o)))

extern signed char D_800FB578;
extern unsigned int D_800D4F0C;
extern unsigned char D_800E78E8[];
extern unsigned short D_8010A3E2;

extern void func_800BFD18(int arg0);

void func_800C0930(int targetId)
{
    int i;
    signed short id = (signed short)targetId;
    signed char count = D_800FB578;

    for (i = 0; (i & 0xFF) < (int)count; i++) {
        unsigned int slot = (unsigned int)(i & 0xFF);
        if ((D_800D4F0C & (1U << slot)) == 0) {
            if (S(D_800E78E8, slot * 0x36) == id) {
                D_8010A3E2 = (unsigned short)slot;
                func_800BFD18(0);
            }
        }
        count = D_800FB578;
    }
}
