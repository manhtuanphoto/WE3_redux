/*
 * func_800B504C
 *
 * Purpose:
 *   PsyQ PopMatrix equivalent. Pops a 32-byte matrix entry from the stack
 *   D_800D2D84 by decrementing byte offset D_800D2D80 by 0x20 and loading the
 *   8 words into PS1 GTE control registers $0..$7 via ctc2. If D_800D2D80 <= 0
 *   (stack empty), logs an underflow error via func_800AD698(D_800D3035)
 *   (saving $ra in D_800D2D74).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Globals read/written:
 *   D_800D2D80 (s32, R/W)     - current matrix stack byte offset
 *   D_800D2D84 (u8[0x280], R) - matrix stack storage (32 bytes per entry)
 *   D_800D2D74 (u32, R/W)     - saved return address across underflow print call
 *   D_800D3035 (u8[], R)      - underflow diagnostic message string
 */

#define W(p, o) (*(unsigned int *)((p) + (o)))

extern signed int D_800D2D80;
extern unsigned char D_800D2D84[];
extern unsigned char D_800D3035[];

extern void func_800AD698(unsigned char *msg);

void func_800B504C(void)
{
    signed int offset = D_800D2D80;
    unsigned char *slot;

    if (offset <= 0) {
        func_800AD698(D_800D3035);
        return;
    }

    offset -= 0x20;
    D_800D2D80 = offset;
    slot = D_800D2D84 + offset;

    /* UNCLEAR: Direct GTE control register writes (ctc2 $0..$7: R11R12, R13R21,
     * R22R23, R31R32, R33, TRX, TRY, TRZ) from slot+0x00..+0x1C.
     * Standard C cannot emit ctc2 without inline assembly or intrinsics. */
    (void)W(slot, 0x00); /* ctc2 $0 */
    (void)W(slot, 0x04); /* ctc2 $1 */
    (void)W(slot, 0x08); /* ctc2 $2 */
    (void)W(slot, 0x0C); /* ctc2 $3 */
    (void)W(slot, 0x10); /* ctc2 $4 */
    (void)W(slot, 0x14); /* ctc2 $5 */
    (void)W(slot, 0x18); /* ctc2 $6 */
    (void)W(slot, 0x1C); /* ctc2 $7 */
}
