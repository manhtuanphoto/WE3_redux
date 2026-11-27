/*
 * func_800B4FA8
 *
 * Purpose:
 *   PsyQ PushMatrix equivalent. Saves the current PS1 GTE rotation matrix and
 *   translation vector (GTE control registers $0..$7 via cfc2) onto the 20-entry
 *   32-byte matrix stack D_800D2D84 at byte offset D_800D2D80, then advances
 *   D_800D2D80 by 0x20. If D_800D2D80 >= 0x280 (stack full), logs an error via
 *   func_800AD698(D_800D3004) instead (saving $ra in D_800D2D74).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Globals read/written:
 *   D_800D2D80 (s32, R/W)     - current matrix stack byte offset (max 0x280 = 20 * 32)
 *   D_800D2D84 (u8[0x280], W) - matrix stack storage (32 bytes per entry)
 *   D_800D2D74 (u32, R/W)     - saved return address across overflow print call
 *   D_800D3004 (u8[], R)      - overflow diagnostic message string
 */

#define W(p, o) (*(unsigned int *)((p) + (o)))

extern signed int D_800D2D80;
extern unsigned char D_800D2D84[];
extern unsigned char D_800D3004[];

extern void func_800AD698(unsigned char *msg);

void func_800B4FA8(void)
{
    signed int offset = D_800D2D80;
    unsigned char *slot;

    if (offset >= 0x280) {
        func_800AD698(D_800D3004);
        return;
    }

    slot = D_800D2D84 + offset;

    /* UNCLEAR: Direct GTE control register reads (cfc2 $0..$7: R11R12, R13R21,
     * R22R23, R31R32, R33, TRX, TRY, TRZ) stored into slot+0x00..+0x1C.
     * Standard C placeholders shown below; requires PS1 GTE cfc2 intrinsics. */
    W(slot, 0x00) = 0; /* cfc2 $0 */
    W(slot, 0x04) = 0; /* cfc2 $1 */
    W(slot, 0x08) = 0; /* cfc2 $2 */
    W(slot, 0x0C) = 0; /* cfc2 $3 */
    W(slot, 0x10) = 0; /* cfc2 $4 */
    W(slot, 0x14) = 0; /* cfc2 $5 */
    W(slot, 0x18) = 0; /* cfc2 $6 */
    W(slot, 0x1C) = 0; /* cfc2 $7 */

    D_800D2D80 = offset + 0x20;
}
