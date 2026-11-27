/*
 * func_800B2A70
 *
 * Purpose:
 *   Transforms, back-face culls, depth-cued lights (GTE NCDS), and links an
 *   array of textured quads (stride 0x20) into the PS1 Ordering Table as
 *   0x28-byte POLY_FT4 GPU packets. Identical to func_800B28C4 except it uses
 *   NCDS instead of NCCS.
 *
 * Arguments:
 *   quads:   (unsigned char*) array of 0x20-byte quad descriptors
 *   verts:   (unsigned char*) array of 8-byte SVECTOR vertices
 *   normals: (unsigned char*) array of 8-byte SVECTOR face normals
 *   outPrim: (unsigned char*) destination GPU primitive buffer (stride 0x28)
 *   count:   (int) number of quads to process
 *   otShift: (unsigned int) right-shift applied to (OTZ - otOffset)
 *   otInfo:  (unsigned char*) OT descriptor (+0x04: u32* otBase, +0x08: s32 otOffset)
 *
 * Return value:
 *   (unsigned char*) updated outPrim pointer after emitted primitives
 *
 * Reads / Writes:
 *   D_8010C1B8: (u32) global semi-transparency / GPU code bit shifted << 1 (R)
 *   GTE Coprocessor 2:
 *     RTPT (V0..V2), NCLIP (MAC0 <= 0 culled), RTPS (V3), AVSZ4 (OTZ),
 *     NCDS (single normal color depth cue using RGBC = 0x808080 | code<<24 -> RGB2)
 */

#define SB(p,o) (*(signed char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))

extern unsigned int D_8010C1B8;

unsigned char *func_800B2A70(unsigned char *quads, unsigned char *verts,
                             unsigned char *normals, unsigned char *outPrim,
                             int count, unsigned int otShift,
                             unsigned char *otInfo) {
    unsigned int *otBase;
    unsigned int otOffset;

    if (count == 0) {
        return outPrim;
    }

    otBase = (unsigned int *)W(otInfo, 0x04);
    otOffset = W(otInfo, 0x08);

    for (; count != 0; count--, quads += 0x20) {
        unsigned int w14 = W(quads, 0x14);
        unsigned int w18 = W(quads, 0x18);
        unsigned short v0Idx = (unsigned short)(w14 >> 16);
        unsigned short n0Idx = (unsigned short)(w14 & 0xFFFFU);
        unsigned short v1Idx = (unsigned short)(w18 & 0xFFFFU);
        unsigned short v2Idx = (unsigned short)(w18 >> 16);
        unsigned short v3Idx = H(quads, 0x1C);
        unsigned int rgbc = (((unsigned int)(SB(quads, 0x03) | (int)(D_8010C1B8 << 1))) << 24) | 0x00808080U;
        unsigned char *v0 = verts + ((unsigned int)v0Idx << 3);
        unsigned char *v1 = verts + ((unsigned int)v1Idx << 3);
        unsigned char *v2 = verts + ((unsigned int)v2Idx << 3);
        unsigned char *v3 = verts + ((unsigned int)v3Idx << 3);
        unsigned char *n0 = normals + ((unsigned int)n0Idx << 3);
        int otz = 0;
        unsigned int litRgbc = 0;

        /* UNCLEAR: PS1 GTE inline coprocessor 2 sequence:
         *   1. Load v0, v1, v2 into GTE V0..V2 and execute RTPT.
         *   2. Load rgbc into GTE RGBC ($6).
         *   3. Check GTE FLAG ($31) < 0 -> skip quad.
         *   4. Execute NCLIP; load v3 into GTE V0; if MAC0 ($24) <= 0 -> skip quad.
         *   5. Store SXY0..2 ($12..$14) to outPrim+0x08, +0x10, +0x18; execute RTPS.
         *   6. Check GTE FLAG ($31) < 0 -> skip quad.
         *   7. Execute AVSZ4; read otz = OTZ ($7); load n0 into GTE V0; execute NCDS.
         *   8. Read litRgbc = RGB2 ($22) and SXY2 ($14) -> outPrim+0x20.
         */
        __asm__ volatile ("" : : : "memory");
        (void)v0; (void)v1; (void)v2; (void)v3; (void)n0; (void)rgbc;

        W(outPrim, 0x0C) = W(quads, 0x04);
        W(outPrim, 0x14) = W(quads, 0x08);
        W(outPrim, 0x1C) = W(quads, 0x0C);
        W(outPrim, 0x24) = W(quads, 0x10);

        {
            unsigned int otIdx = ((unsigned int)(otz - (int)otOffset) >> otShift) & 0xFFFFU;
            unsigned int *otEntry = &otBase[otIdx];
            unsigned int oldHead = *otEntry;

            W(outPrim, 0x00) = 0x09000000U | (oldHead & 0x00FFFFFFU);
            *otEntry = (oldHead & 0xFF000000U) | ((unsigned int)outPrim & 0x00FFFFFFU);
            W(outPrim, 0x04) = litRgbc;
        }

        outPrim += 0x28;
    }

    return outPrim;
}
