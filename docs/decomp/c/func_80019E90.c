/*
 * func_80019E90
 *
 * Purpose:
 *   Transforms a 4-vertex quad from local space into screen space using the
 *   rotation angles at Scratchpad+0x124, world translation at Scratchpad+0x13C,
 *   and camera matrix/translation at Scratchpad+0x1A8..0x1C4 via the PS1 GTE
 *   (MVMVA + RTPT + RTPS), writing the four resulting screen XY words to
 *   outPrim + 0x08, +0x10, +0x18, and +0x20.
 *
 * Arguments:
 *   verts:   (unsigned char*) pointer to 4 SVECTOR vertices (stride 8 bytes)
 *   outPrim: (unsigned char*) pointer to destination GPU primitive struct
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x104..+0x114 (9 x s16) rotation matrix workspace (R/W)
 *     +0x118..+0x120 (3 x s32) translation workspace (R/W)
 *     +0x124..+0x128 (3 x s16) local rotation angles (R)
 *     +0x13C..+0x144 (3 x s32) local world translation (R)
 *     +0x1A8..+0x1C4 camera rotation matrix + translation (R)
 *   outPrim:
 *     +0x08, +0x10, +0x18, +0x20 (u32) projected screen XY coordinates (W)
 *
 * Magic numbers:
 *   None (GTE matrix/vector transform and perspective projection).
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern void func_800B2178(unsigned char* rot, unsigned char* outMat);

void func_80019E90(unsigned char* verts, unsigned char* outPrim) {
    unsigned short m00, m01, m02, m10, m11, m12, m20, m21, m22;
    unsigned int tx, ty, tz;

    func_800B2178(Scratchpad + 0x124, Scratchpad + 0x104);

    m00 = H(Scratchpad, 0x104);
    m01 = H(Scratchpad, 0x106);
    m02 = H(Scratchpad, 0x108);
    m10 = H(Scratchpad, 0x10A);
    m11 = H(Scratchpad, 0x10C);
    m12 = H(Scratchpad, 0x10E);
    m20 = H(Scratchpad, 0x110);
    m21 = H(Scratchpad, 0x112);
    m22 = H(Scratchpad, 0x114);
    tx = W(Scratchpad, 0x13C);
    ty = W(Scratchpad, 0x140);
    tz = W(Scratchpad, 0x144);

    H(Scratchpad, 0x104) = m00;
    H(Scratchpad, 0x106) = m01;
    H(Scratchpad, 0x108) = m02;
    H(Scratchpad, 0x10A) = m10;
    H(Scratchpad, 0x10C) = m11;
    H(Scratchpad, 0x10E) = m12;
    H(Scratchpad, 0x110) = m20;
    H(Scratchpad, 0x112) = m21;
    H(Scratchpad, 0x114) = m22;
    W(Scratchpad, 0x118) = tx;
    W(Scratchpad, 0x11C) = ty;
    W(Scratchpad, 0x120) = tz;

    /* UNCLEAR: PS1 GTE inline coprocessor 2 sequence:
     *   1. Loads camera rotation (Scratchpad+0x1A8..0x1B8) into GTE R11..R33
     *      and multiplies each of the 3 columns of Scratchpad+0x104 via
     *      mvmva(1,0,3,3,0), storing the resulting rotation matrix back to
     *      Scratchpad+0x104..0x114.
     *   2. Loads camera translation (Scratchpad+0x1BC..0x1C4) into GTE TRX/Y/Z
     *      and transforms translation vector Scratchpad+0x118..0x120 via
     *      mvmva(1,0,0,0,0), storing MAC1..3 back to Scratchpad+0x118..0x120.
     *   3. Loads the combined translation (Scratchpad+0x118..0x120) and matrix
     *      (Scratchpad+0x104..0x114) into GTE TR/R, runs RTPT on verts[0..2]
     *      writing SXY0..2 to outPrim+0x08/+0x10/+0x18, then runs RTPS on
     *      verts[3] (verts+0x18) writing SXY2 to outPrim+0x20.
     */
    __asm__ volatile ("" : : : "memory");
    (void)verts;
    (void)outPrim;
}
