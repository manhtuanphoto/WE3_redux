/*
 * func_8009A8FC
 *
 * Purpose:
 *   Transforms and submits a 20-quad strip mesh (2 strips of 10 quads, e.g.
 *   goal net or banner/flag) via PS1 GTE. Computes the model-view matrix from
 *   the object's Y rotation and X/Z position, transforms quad corners from
 *   D_800CA968 (reusing shared bottom vertices across adjacent strip quads),
 *   applies GTE NCLIP back-face culling, and inserts visible primitives into
 *   ordering table D_800F3568.
 *
 * Arguments:
 *   obj - pointer to renderable object struct (+0x08/+0x0C s16 x/z, +0x16 s16 rotY, +0x8C u8 slot)
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Object:
 *     +0x008 (u16) x position (R)
 *     +0x00C (u16) z position (R)
 *     +0x016 (u16) Y rotation (R)
 *     +0x08C (u8)  primitive slot index (minus 10, stride 0x320) (R)
 *   Scratchpad (0x1F800000):
 *     +0x000 (s32) OT shift base (R)
 *     +0x0EC (s32) NCLIP MAC0 result / OT Z depth (SZ3 >> 2) (R/W)
 *     +0x104..+0x120 (MATRIX) combined GTE rotation + translation (R/W)
 *     +0x124..+0x130 (SVECTOR/VECTOR) rotation & translation inputs (W)
 *     +0x13C..+0x144 (s32[3]) transformed translation (R/W)
 *     +0x14C..+0x154 (s32[3]) scale vector = 0x1000 (W)
 *     +0x190 (u32) cached pointer to D_800E8208 (R/W)
 *     +0x1A8..+0x1C4 (MATRIX) camera matrix + translation (R)
 *     +0x29D (u8)  display buffer index (R)
 *   Globals:
 *     D_800CA918..D_800CA91B (u8[4] per quad) 4 vertex indices per quad (R)
 *     D_800CA968: vertex array (8 bytes per SVECTOR) (R)
 *     D_800E8208: primitive buffer (stride 0x3AC0 per buffer; +0x23F0 + (slot-10)*0x320) (R/W)
 *     D_800F3568: ordering table (stride 0x4000 per buffer) (R/W)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x1000 = 1.0 fixed-point scale
 *   0x3AC0 = 15040-byte primitive buffer stride per display buffer
 *   0x320 = 800 bytes = 20 primitives * 0x28 bytes per object slot
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800CA918[];
extern unsigned char D_800CA919[];
extern unsigned char D_800CA91A[];
extern unsigned char D_800CA91B[];
extern unsigned char D_800CA968[];
extern unsigned char D_800E8208[];
extern unsigned char D_800F3568[];

extern void func_800B1488(unsigned char*, unsigned char*);
extern void func_800B1348(unsigned char*, unsigned char*);
extern void func_800B11B8(unsigned char*, unsigned char*);
extern void func_800B12C8(unsigned char*, unsigned char*, unsigned char*);
extern void func_800ADC04(unsigned char*, unsigned char*);

void func_8009A8FC(unsigned char* obj) {
    int slotOffset;
    int i;
    int primOff;
    int tupleOff;
    unsigned short prevX0 = 0;
    unsigned short prevY0 = 0;
    unsigned short prevX1 = 0;
    unsigned short prevY1 = 0;

    H(Scratchpad, 0x124) = 0;
    H(Scratchpad, 0x128) = 0;
    H(Scratchpad, 0x126) = H(obj, 0x16);
    H(Scratchpad, 0x12E) = 0;
    H(Scratchpad, 0x12C) = H(obj, 0x8);
    W(Scratchpad, 0x14C) = 0x1000;
    W(Scratchpad, 0x150) = 0x1000;
    W(Scratchpad, 0x154) = 0x1000;
    H(Scratchpad, 0x130) = H(obj, 0xC);

    func_800B1488(Scratchpad + 0x124, Scratchpad + 0x104);
    func_800B1348(Scratchpad + 0x104, Scratchpad + 0x14C);
    func_800B11B8(Scratchpad + 0x1A8, Scratchpad + 0x104);
    func_800B12C8(Scratchpad + 0x1A8, Scratchpad + 0x12C, Scratchpad + 0x13C);

    SW(Scratchpad, 0x118) = SW(Scratchpad, 0x13C) + SW(Scratchpad, 0x1BC);
    SW(Scratchpad, 0x11C) = SW(Scratchpad, 0x140) + SW(Scratchpad, 0x1C0);
    SW(Scratchpad, 0x120) = SW(Scratchpad, 0x144) + SW(Scratchpad, 0x1C4);

    /* UNCLEAR: PS1 GTE COP2 setup: loads rotation matrix and translation
     * from Scratchpad+0x104..0x120 into GTE control registers $0..$7 (ctc2). */
    __asm__ volatile ("" : : : "memory");

    W(Scratchpad, 0x190) = (unsigned int)D_800E8208;
    slotOffset = ((int)B(obj, 0x8C) - 10) * 0x320 + 0x23F0;

    primOff = 0;
    tupleOff = 0;
    for (i = 0; i < 20; i++) {
        unsigned int bufOff = (unsigned int)B(Scratchpad, 0x29D) * 0x3AC0u;
        unsigned char* prim = (unsigned char*)W(Scratchpad, 0x190) + bufOff + slotOffset + (unsigned int)primOff;
        int nclipVal;

        if (i == 0 || i == 10) {
            /* UNCLEAR: GTE RTPS transforms D_800CA968[D_800CA918[tupleOff]] to prim+0x08
             * and D_800CA968[D_800CA919[tupleOff]] to prim+0x10 (SXY2 -> $14). */
            (void)D_800CA968[B(D_800CA918, tupleOff) * 8u];
            (void)D_800CA968[B(D_800CA919, tupleOff) * 8u];
            __asm__ volatile ("" : : : "memory");
        } else {
            H(prim, 0x08) = prevX0;
            H(prim, 0x0A) = prevY0;
            H(prim, 0x10) = prevX1;
            H(prim, 0x12) = prevY1;
        }

        /* UNCLEAR: GTE RTPS transforms D_800CA968[D_800CA91A[tupleOff]] to prim+0x18
         * and D_800CA968[D_800CA91B[tupleOff]] to prim+0x20 (SXY2 -> $14). */
        (void)D_800CA968[B(D_800CA91A, tupleOff) * 8u];
        (void)D_800CA968[B(D_800CA91B, tupleOff) * 8u];
        __asm__ volatile ("" : : : "memory");

        prevX0 = H(prim, 0x18);
        prevY0 = H(prim, 0x1A);
        prevX1 = H(prim, 0x20);
        prevY1 = H(prim, 0x22);

        /* UNCLEAR: GTE NCLIP checks screen-space winding; stores MAC0 ($24) to Scratchpad+0xEC. */
        __asm__ volatile ("" : : : "memory");
        nclipVal = SW(Scratchpad, 0x0EC);

        if (nclipVal <= 0) {
            /* UNCLEAR: GTE reads SZ3 ($19) >> 2 into Scratchpad+0xEC. */
            int otZ;
            __asm__ volatile ("" : : : "memory");
            otZ = SW(Scratchpad, 0x0EC);
            if (otZ > 0) {
                unsigned int otShift = 14u - W(Scratchpad, 0x000);
                unsigned int otIndex = (unsigned int)(otZ >> otShift);
                unsigned char* otEntry = D_800F3568 + ((unsigned int)B(Scratchpad, 0x29D) << 14) + (otIndex << 2);
                func_800ADC04(otEntry, prim);
            }
        }

        tupleOff += 4;
        primOff += 0x28;
    }
}
