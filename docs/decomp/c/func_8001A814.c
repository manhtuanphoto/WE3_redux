/*
 * func_8001A814
 *
 * Purpose:
 *   Transforms and submits a single GPU quad primitive (at +0x50 of a player's
 *   0xA0-byte primitive slot in D_800E8208) into the PS1 Ordering Table
 *   (D_800F3560) using zero local rotation and the actor's (x, 0, z) world
 *   position via func_80019E90.
 *
 * Arguments:
 *   actor: (unsigned char*) pointer to player entity struct
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x000 (u32) OT Z-depth shift index (R)
 *     +0x124..+0x128 (3 x s16) local rotation angles cleared to 0 (W)
 *     +0x13C..+0x144 (3 x s32) local translation (actor x, 0, z) (W)
 *     +0x29D (u8)  double-buffer frame index (0 or 1) (R)
 *   D_800E8208: primitive buffer (0x3AC0 per frame, 0xA0 per player, +0x50 quad) (W)
 *   D_800F3560: Ordering Table base (0x4000 per frame) (R/W)
 *   D_80106B6C: quad vertex table (0x80 per player) (R)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800E8208[];
extern unsigned char D_800F3560[];
extern unsigned char D_80106B6C[];

extern void func_80019E90(unsigned char *verts, unsigned char *outPrim);

void func_8001A814(unsigned char *actor) {
    int playerIdx = (int)B(actor, 0x38D) - 1;
    unsigned char bufIdx = B(Scratchpad, 0x29D);
    unsigned char *framePrims = D_800E8208 + (bufIdx * 0x3AC0);
    unsigned char *outPrim = framePrims + (playerIdx * 0xA0) + 0x50;
    unsigned char *verts = D_80106B6C + (playerIdx << 7);
    unsigned int *otEntry;

    H(Scratchpad, 0x124) = 0;
    H(Scratchpad, 0x126) = 0;
    H(Scratchpad, 0x128) = 0;
    SW(Scratchpad, 0x13C) = S(actor, 0x02);
    SW(Scratchpad, 0x140) = 0;
    SW(Scratchpad, 0x144) = S(actor, 0x06);

    func_80019E90(verts, outPrim);

    otEntry = (unsigned int *)(D_800F3560 + (B(Scratchpad, 0x29D) << 14) + (4U << W(Scratchpad, 0x000)));
    W(outPrim, 0x00) = (W(outPrim, 0x00) & 0xFF000000U) | (*otEntry & 0x00FFFFFFU);
    *otEntry = (*otEntry & 0xFF000000U) | ((unsigned int)outPrim & 0x00FFFFFFU);
}
