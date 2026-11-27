/*
 * func_8009B4F8
 *
 * Purpose:
 *   Builds a quad packet's transformed 2D screen coordinates via the PS1 GTE
 *   for a player marker/shadow/billboard sprite at (x, y, z) and inserts it
 *   into the ordering table (D_800F3568) when in front of the camera (otz > 0).
 *
 * Arguments:
 *   posX (u16 *): pointer to X coordinate (16-bit)
 *   posY (u16 *): pointer to Y coordinate (16-bit)
 *   posZ (u16 *): pointer to Z coordinate (16-bit)
 *   slot (u8):    marker/player slot index
 *   arg4 (u8 *):  sprite descriptor pointer (first byte indexes D_800CB168)
 *   arg5 (int):   forwarded to func_8009B36C
 *
 * Return value:
 *   void
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x000 (s32) OT depth shift divisor (shift = 14 - Scratchpad+0x000)
 *   +0x0EC (s32) GTE SZ3 >> 2 depth value
 *   +0x104..+0x123 GTE transformation matrix
 *   +0x13C..+0x144 translation vector
 *   +0x1A8..+0x1C7 view matrix
 *   +0x29D (u8)  active double-buffer index (0 or 1)
 *
 * Scratchpad fields written:
 *   +0x0EC (s32) GTE SZ3 >> 2
 *   +0x118, +0x11C, +0x120 (s32) view-space translation
 *   +0x124..+0x128 (s16) zeroed rotation
 *   +0x12C, +0x12E, +0x130 (u16) input posX/posY/posZ
 *   +0x14C, +0x150, +0x154 (s32) unit scale 0x1000
 *   +0x190 (u32) D_800E8208 base pointer
 *
 * Global fields read/written:
 *   D_800CB168[] (s16 *[]), D_800E6E2D[] (u8 stride 16),
 *   D_800CB2CC..D_800CB304 (vertex quad templates),
 *   D_800E8208[] (packet buffer, stride 0x3AC0 per frame buffer, 0x28 per slot),
 *   D_800F3568[] (ordering table, stride 0x4000 per frame buffer).
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern signed short *D_800CB168[];
extern unsigned char D_800CB2CC[];
extern unsigned char D_800CB2D4[];
extern unsigned char D_800CB2DC[];
extern unsigned char D_800CB2E4[];
extern unsigned char D_800CB2EC[];
extern unsigned char D_800CB2F4[];
extern unsigned char D_800CB2FC[];
extern unsigned char D_800CB304[];
extern unsigned char D_800E6E2D[];
extern unsigned char D_800E8208[];
extern unsigned char D_800F3568[];

extern void func_8009B36C(unsigned char *arg4, unsigned char *slotPtr, int arg5);
extern void func_800B1348(unsigned char *mat, unsigned char *scale);
extern void func_800B19A8(unsigned char *rot, unsigned char *mat);
extern void func_800B12C8(unsigned char *mat, unsigned char *vec, unsigned char *out);
extern void func_800ADC04(unsigned char *otEntry, unsigned char *prim);

void func_8009B4F8(unsigned short *posX, unsigned short *posY, unsigned short *posZ,
                   unsigned char slot, unsigned char *arg4, int arg5)
{
    unsigned char localSlot = slot;
    signed short *xOffPtr;
    unsigned char bufIdx;
    unsigned char *pkt;
    int otz;

    func_8009B36C(arg4, &localSlot, arg5);
    xOffPtr = D_800CB168[B(arg4, 0)];

    S(Scratchpad, 0x124) = 0;
    S(Scratchpad, 0x126) = 0;
    S(Scratchpad, 0x128) = 0;
    SW(Scratchpad, 0x14C) = 0x1000;
    SW(Scratchpad, 0x150) = 0x1000;
    SW(Scratchpad, 0x154) = 0x1000;

    H(Scratchpad, 0x12C) = *posX;
    H(Scratchpad, 0x12E) = *posY;
    H(Scratchpad, 0x130) = *posZ;

    func_800B1348(Scratchpad + 0x104, Scratchpad + 0x14C);
    func_800B19A8(Scratchpad + 0x124, Scratchpad + 0x104);
    func_800B12C8(Scratchpad + 0x1A8, Scratchpad + 0x12C, Scratchpad + 0x13C);

    SW(Scratchpad, 0x118) = SW(Scratchpad, 0x13C) + xOffPtr[0] + SW(Scratchpad, 0x1BC);
    SW(Scratchpad, 0x11C) = SW(Scratchpad, 0x140) + SW(Scratchpad, 0x1C0);
    SW(Scratchpad, 0x120) = SW(Scratchpad, 0x144) + SW(Scratchpad, 0x1C4);

    /* GTE SetRotMatrix / SetTransMatrix from Scratchpad+0x104..0x123 */
    bufIdx = B(Scratchpad, 0x29D);
    W(Scratchpad, 0x190) = (unsigned int)D_800E8208;
    pkt = D_800E8208 + (unsigned int)bufIdx * 0x3AC0 + (unsigned int)localSlot * 0x28 + 0x3660;

    /* UNCLEAR: GTE inline rtpt/rtps transform using Scratchpad+0x104 matrix;
     * writes screen XY pairs (SXY0/SXY1/SXY2) to pkt+0x08, +0x10, +0x18 and
     * fourth vertex SXY2 to pkt+0x20, leaving SZ3 in GTE register $19. */
    if (D_800E6E2D[localSlot << 4] != 0) {
        /* GTE rtpt(D_800CB2F4, D_800CB2FC, D_800CB2EC) -> pkt+0x08, +0x10, +0x18 */
        /* GTE rtps(D_800CB304) -> pkt+0x20 */
        (void)D_800CB2F4;
        (void)D_800CB2FC;
        (void)D_800CB2EC;
        (void)D_800CB304;
    } else {
        /* GTE rtpt(D_800CB2D4, D_800CB2DC, D_800CB2CC) -> pkt+0x08, +0x10, +0x18 */
        /* GTE rtps(D_800CB2E4) -> pkt+0x20 */
        (void)D_800CB2D4;
        (void)D_800CB2DC;
        (void)D_800CB2CC;
        (void)D_800CB2E4;
    }

    /* SW(Scratchpad, 0x0EC) = gte_SZ3 >> 2; */
    otz = SW(Scratchpad, 0x0EC);
    if (otz > 0) {
        int shiftedOtz = otz >> (14 - SW(Scratchpad, 0x000));
        int otIdx = (shiftedOtz * 37) / 40;

        func_800ADC04(D_800F3568 + ((unsigned int)B(Scratchpad, 0x29D) << 14) + otIdx * 4, pkt);
    }
}
