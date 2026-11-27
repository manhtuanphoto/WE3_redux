/*
 * func_8009BA54
 *
 * Purpose:
 *   Builds a local-to-camera transformation matrix from a rotation lookup
 *   (D_800CADF0[*arg4]), position (arg0, arg1, arg2), and handedness/tilt flag
 *   (arg3: +/-0x400 Y-rot, -/+0x300 Z-rot), then transforms 5 textured quads
 *   from D_800CB250 (indexed by D_800CB290..D_800CB293) via PS1 GTE RTPT+RTPS
 *   into 40-byte primitive slots in D_800E8208 (starting at slot arg5) and
 *   inserts visible primitives into ordering table D_800F3568.
 *
 * Arguments:
 *   arg0: (signed short) world X position
 *   arg1: (signed short) world Y position
 *   arg2: (signed short) world Z position
 *   arg3: (u8) orientation/side flag (0 vs non-zero)
 *   arg4: (unsigned char*) pointer to rotation table index in D_800CADF0
 *   arg5: (u8) starting primitive slot index in D_800E8208
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x000 (s32) OT depth shift base (R)
 *     +0x0EC (s32) computed OT Z (SZ3 >> 2) (W/R)
 *     +0x104..+0x120 (matrix + translation) (R/W)
 *     +0x124..+0x128 (3 x s16) rotation input (W)
 *     +0x12C..+0x130 (3 x s16) world position input (W)
 *     +0x13C..+0x144 (3 x s32) transformed translation (R)
 *     +0x14C..+0x154 (3 x s32) scale input (W = 0x1000)
 *     +0x190 (u32) pointer to D_800E8208 (W/R)
 *     +0x1A8..+0x1C4 camera matrix + translation (R)
 *     +0x29D (u8) display buffer index (0/1) (R)
 *   Globals:
 *     D_800CADF0, D_800CB250, D_800CB290..D_800CB293 (R)
 *     D_800E8208 (buffer stride 0x3AC0, base +0x31B0, slot stride 40) (W)
 *     D_800F3568 (ordering table, buffer stride 0x4000) (R/W)
 *
 * Magic numbers:
 *   0x1000 = 1.0 fixed-point scale
 *   0x400 / 0x300 = 90 deg / 67.5 deg in 4096-per-rev GTE angles
 *   0x3AC0 = 15040-byte primitive buffer stride per display buffer
 *   0x31B0 = byte offset into D_800E8208 for the 5-quad primitive pool
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char* D_800CADF0[];
extern unsigned char D_800E8208[];
extern unsigned char D_800CB250[];
extern unsigned char D_800CB290[];
extern unsigned char D_800CB291[];
extern unsigned char D_800CB292[];
extern unsigned char D_800CB293[];
extern unsigned char D_800F3568[];

extern void func_800B19A8(unsigned char* rot, unsigned char* outMat);
extern void func_800B1C38(int angle, unsigned char* mat);
extern void func_800B1DD8(int angle, unsigned char* mat);
extern void func_800B1348(unsigned char* mat, unsigned char* scale);
extern void func_800B11B8(unsigned char* camMat, unsigned char* objMat);
extern void func_800B12C8(unsigned char* camMat, unsigned char* pos, unsigned char* outVec);
extern void func_800ADC04(unsigned char* otEntry, unsigned char* prim);

void func_8009BA54(int arg0, int arg1, int arg2, unsigned int arg3, unsigned char* arg4, unsigned int arg5) {
    unsigned char* rotEntry = D_800CADF0[*arg4];
    unsigned int baseSlot = arg5 & 0xFFU;
    int i;

    H(Scratchpad, 0x124) = H(rotEntry, 0x0);
    H(Scratchpad, 0x126) = H(rotEntry, 0x2);
    H(Scratchpad, 0x12C) = (unsigned short)arg0;
    H(Scratchpad, 0x12E) = (unsigned short)arg1;
    H(Scratchpad, 0x130) = (unsigned short)arg2;
    W(Scratchpad, 0x14C) = 0x1000;
    W(Scratchpad, 0x150) = 0x1000;
    W(Scratchpad, 0x154) = 0x1000;
    H(Scratchpad, 0x128) = H(rotEntry, 0x4);

    func_800B19A8(Scratchpad + 0x124, Scratchpad + 0x104);

    if ((arg3 & 0xFFU) == 0) {
        func_800B1C38(-0x400, Scratchpad + 0x104);
        func_800B1DD8(0x300, Scratchpad + 0x104);
    } else {
        func_800B1C38(0x400, Scratchpad + 0x104);
        func_800B1DD8(-0x300, Scratchpad + 0x104);
    }

    func_800B1348(Scratchpad + 0x104, Scratchpad + 0x14C);
    func_800B11B8(Scratchpad + 0x1A8, Scratchpad + 0x104);
    func_800B12C8(Scratchpad + 0x1A8, Scratchpad + 0x12C, Scratchpad + 0x13C);

    SW(Scratchpad, 0x118) = SW(Scratchpad, 0x13C) + SW(Scratchpad, 0x1BC);
    SW(Scratchpad, 0x11C) = SW(Scratchpad, 0x140) + SW(Scratchpad, 0x1C0);
    SW(Scratchpad, 0x120) = SW(Scratchpad, 0x144) + SW(Scratchpad, 0x1C4);

    /* UNCLEAR: PS1 GTE inline coprocessor 2 matrix/translation load:
     *   Loads Scratchpad+0x104..0x114 into GTE R11..R33 and
     *   Scratchpad+0x118..0x120 into GTE TRX/TRY/TRZ. */
    __asm__ volatile ("" : : : "memory");

    W(Scratchpad, 0x190) = (unsigned int)D_800E8208;

    for (i = 0; (i & 0xFF) < 5; i++) {
        unsigned int slot = (baseSlot + (unsigned int)i) & 0xFFU;
        unsigned int bufIdx = B(Scratchpad, 0x29D);
        unsigned char* prim = (unsigned char*)W(Scratchpad, 0x190) + bufIdx * 0x3AC0U + slot * 40U + 0x31B0U;
        unsigned int idxOff = (unsigned int)(i & 0xFF) * 4U;
        unsigned char* v0 = D_800CB250 + B(D_800CB290, idxOff) * 8U;
        unsigned char* v1 = D_800CB250 + B(D_800CB291, idxOff) * 8U;
        unsigned char* v2 = D_800CB250 + B(D_800CB292, idxOff) * 8U;
        unsigned char* v3 = D_800CB250 + B(D_800CB293, idxOff) * 8U;
        int otZ;

        /* UNCLEAR: PS1 GTE inline coprocessor 2 projection:
         *   Runs RTPT on (v0, v1, v2), writing SXY0..2 to prim+0x08/+0x10/+0x18,
         *   then runs RTPS on v3, writing SXY2 to prim+0x20 and SZ3 >> 2 to
         *   Scratchpad+0xEC. */
        __asm__ volatile ("" : : : "memory");
        (void)v0; (void)v1; (void)v2; (void)v3;

        otZ = SW(Scratchpad, 0x0EC);
        if (otZ > 0) {
            int otDepth = (otZ >> (14 - SW(Scratchpad, 0x000))) * 19 / 20;
            unsigned char* otEntry = D_800F3568 + ((unsigned int)B(Scratchpad, 0x29D) << 14) + otDepth * 4;
            func_800ADC04(otEntry, prim);
        }
    }
}
