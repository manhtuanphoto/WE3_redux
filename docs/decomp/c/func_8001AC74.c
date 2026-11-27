/*
 * func_8001AC74
 *
 * Purpose:
 *   Builds the ground-projected quad (landing/target marker) at the predicted
 *   ball target (Scratchpad+0x2B8, 0, Scratchpad+0x2BA), transforms its four
 *   template corners from Scratchpad+0x268..0x280 into screen space via the PS1
 *   GTE (RTPT + RTPS), stores the resulting screen coordinates into the active
 *   primitive slot in D_800E8208, and inserts the primitive into the appropriate
 *   ordering table (D_800F3468 or D_800F3560).
 *
 * Arguments:
 *   void
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x000 (u32) OT depth shift base (R)
 *     +0x0EC (s32) computed OT Z index (SZ3 >> 2) (W/R)
 *     +0x0F6 (u8)  OT mode selector (non-zero -> D_800F3468, 0 -> D_800F3560) (R)
 *     +0x104..+0x120 (matrix + translation workspace) (R/W)
 *     +0x124..+0x128 (s16[3]) rotation input (W = 0)
 *     +0x13C..+0x144 (s32[3]) world position input (W)
 *     +0x14C..+0x154 (s32[3]) scale input (W = 0x1C92)
 *     +0x1A8..+0x1C4 (camera matrix + translation) (R)
 *     +0x268, +0x270, +0x278, +0x280 (SVECTOR[4]) quad corner vertices (R)
 *     +0x29D (u8)  display buffer index (0/1) (R)
 *     +0x2B8 (s16) predicted ball target X (0x7FFF = inactive -> early return) (R)
 *     +0x2BA (s16) predicted ball target Z (R)
 *   Globals:
 *     D_800E8208: primitive buffer (stride 0x3AC0 = 15040 per buffer; +0x28 tag,
 *       +0x30/+0x38/+0x40/+0x48 screen XY outputs) (W)
 *     D_800F3468 / D_800F3560: ordering tables (stride 0x4000 per buffer) (R/W)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x7FFF = invalid/no target sentinel
 *   0x1C92 = 7314 (~1.786 in 4096 = 1.0 fixed-point) marker scale
 *   0x3AC0 = 15040-byte primitive table stride per display buffer
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800E8208[];
extern unsigned char D_800F3468[];
extern unsigned char D_800F3560[];

extern void func_800B2178(unsigned char*, unsigned char*);
extern void func_800B1348(unsigned char*, unsigned char*);
extern void func_800B1318(unsigned char*, unsigned char*);

void func_8001AC74(void) {
    int targetX = S(Scratchpad, 0x2B8);
    unsigned char* prim;
    unsigned char bufIdx;
    int otZ;

    if (targetX == 0x7FFF) {
        return;
    }

    SW(Scratchpad, 0x13C) = targetX;
    H(Scratchpad, 0x124) = 0;
    H(Scratchpad, 0x126) = 0;
    H(Scratchpad, 0x128) = 0;
    SW(Scratchpad, 0x140) = 0;
    SW(Scratchpad, 0x14C) = 0x1C92;
    SW(Scratchpad, 0x150) = 0x1C92;
    SW(Scratchpad, 0x154) = 0x1C92;
    SW(Scratchpad, 0x144) = S(Scratchpad, 0x2BA);

    func_800B2178(Scratchpad + 0x124, Scratchpad + 0x104);
    func_800B1348(Scratchpad + 0x104, Scratchpad + 0x14C);
    func_800B1318(Scratchpad + 0x104, Scratchpad + 0x13C);

    bufIdx = B(Scratchpad, 0x29D);
    prim = D_800E8208 + (bufIdx * 0x3AC0);

    /* UNCLEAR: PS1 GTE inline coprocessor 2 sequence:
     *   1. Loads camera rotation (Scratchpad+0x1A8) and multiplies each column
     *      of Scratchpad+0x104 via mvmva(1,0,3,3,0), storing back to +0x104.
     *   2. Loads camera translation (Scratchpad+0x1BC) and transforms translation
     *      vector Scratchpad+0x118 via mvmva(1,0,0,0,0), storing back to +0x118.
     *   3. Loads the combined matrix/translation from Scratchpad+0x104..0x120
     *      into GTE R/TR, runs RTPT on Scratchpad+0x268/+0x270/+0x278 writing
     *      SXY0..2 to prim+0x30/0x38/0x40, then runs RTPS on Scratchpad+0x280
     *      writing SXY2 to prim+0x48 and SZ3 >> 2 to Scratchpad+0xEC.
     */
    __asm__ volatile ("" : : : "memory");
    otZ = SW(Scratchpad, 0x0EC);

    if (B(Scratchpad, 0x0F6) != 0) {
        unsigned int otOff = (unsigned int)((otZ >> 2) << 2) + ((unsigned int)B(Scratchpad, 0x29D) << 14);
        unsigned int* otEntry = (unsigned int*)(D_800F3468 + otOff);
        W(prim, 0x28) = (W(prim, 0x28) & 0xFF000000u) | (*otEntry & 0x00FFFFFFu);
        otOff = (unsigned int)((SW(Scratchpad, 0x0EC) >> 2) << 2) + ((unsigned int)B(Scratchpad, 0x29D) << 14);
        otEntry = (unsigned int*)(D_800F3468 + otOff);
        *otEntry = (*otEntry & 0xFF000000u) | (((unsigned int)(prim + 0x28)) & 0x00FFFFFFu);
    } else if (otZ > 0) {
        unsigned int shift = W(Scratchpad, 0x000);
        unsigned int otOff = (4u << shift) + ((unsigned int)B(Scratchpad, 0x29D) << 14);
        unsigned int* otEntry = (unsigned int*)(D_800F3560 + otOff);
        W(prim, 0x28) = (W(prim, 0x28) & 0xFF000000u) | (*otEntry & 0x00FFFFFFu);
        otOff = (4u << W(Scratchpad, 0x000)) + ((unsigned int)B(Scratchpad, 0x29D) << 14);
        otEntry = (unsigned int*)(D_800F3560 + otOff);
        *otEntry = (*otEntry & 0xFF000000u) | (((unsigned int)(prim + 0x28)) & 0x00FFFFFFu);
    }
}
