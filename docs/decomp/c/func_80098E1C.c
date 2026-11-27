/*
 * func_80098E1C
 *
 * Purpose:
 *   Transforms and submits a 20-triangle mesh (e.g. corner flag / net part)
 *   for rendering via PS1 GTE. Computes the model-view matrix from the
 *   object's rotation and position, transforms 20 shared vertices from
 *   D_800E6D90 to screen coordinates, builds 20 primitives (stride 0x20) in
 *   D_800E8208, and inserts visible primitives into ordering table D_800F3568.
 *
 * Arguments:
 *   obj - pointer to renderable object struct (+0x08/+0x0C s16 x/z, +0x14..+0x18 s16 rot, +0x8C u8 slot)
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Object:
 *     +0x008 (u16) x position (R)
 *     +0x00C (u16) z position (R)
 *     +0x014/+0x016/+0x018 (u16) rotation X/Y/Z (R; Y += 0x800)
 *     +0x08C (u8)  primitive slot index (minus 8, stride 0x280) (R)
 *   Scratchpad (0x1F800000):
 *     +0x000 (s32) OT shift base (R)
 *     +0x0EC (s32) computed OT Z depth (SZ3 >> 2) (R/W)
 *     +0x104..+0x120 (MATRIX) combined GTE rotation + translation (R/W)
 *     +0x124..+0x130 (SVECTOR/VECTOR) rotation & translation inputs (W)
 *     +0x13C..+0x144 (s32[3]) transformed translation (R/W)
 *     +0x14C..+0x154 (s32[3]) scale vector = 0x1000 (W)
 *     +0x190 (u32) cached pointer to D_800E8208 (R/W)
 *     +0x1A8..+0x1C4 (MATRIX) camera matrix + translation (R)
 *     +0x29D (u8)  display buffer index (R)
 *   Globals:
 *     D_800E6D90: vertex array (8 bytes per SVECTOR) (R)
 *     D_800CA774 / D_800CA778: triangle index table (6 bytes = 3 s16 indices per tri) (R)
 *     D_800E8208: primitive buffer (stride 0x3AC0 per buffer; +0xEB0 + (slot-8)*0x280) (R/W)
 *     D_800F3568: ordering table (stride 0x4000 per buffer) (R/W)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x800 = 180 deg Y rotation offset (4096 = 360 deg)
 *   0x1000 = 1.0 fixed-point scale
 *   0x3AC0 = 15040-byte primitive buffer stride per display buffer
 *   0x280 = 640 bytes = 20 primitives * 0x20 bytes per object slot
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800CA774[];
extern unsigned char D_800CA778[];
extern unsigned char D_800E6D90[];
extern unsigned char D_800E8208[];
extern unsigned char D_800F3568[];

extern void func_800B1488(unsigned char*, unsigned char*);
extern void func_800B1348(unsigned char*, unsigned char*);
extern void func_800B11B8(unsigned char*, unsigned char*);
extern void func_800B12C8(unsigned char*, unsigned char*, unsigned char*);
extern void func_800ADC04(unsigned char*, unsigned char*);

void func_80098E1C(unsigned char* obj) {
    unsigned short screenX[20];
    unsigned short screenY[20];
    int slotOffset;
    unsigned char* slotBase;
    int i;

    H(Scratchpad, 0x124) = H(obj, 0x14);
    H(Scratchpad, 0x126) = (unsigned short)(H(obj, 0x16) + 0x800u);
    H(Scratchpad, 0x128) = H(obj, 0x18);
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
    slotOffset = ((int)B(obj, 0x8C) - 8) * 0x280 + 0xEB0;
    slotBase = D_800E8208 + (unsigned int)B(Scratchpad, 0x29D) * 0x3AC0u + slotOffset;

    for (i = 0; i < 20; i++) {
        /* UNCLEAR: GTE RTPS transforms D_800E6D90[i] (8-byte SVECTOR) and writes
         * SXY2 ($14) into slotBase + 8 before caching XY into screenX/screenY. */
        (void)D_800E6D90[i * 8];
        __asm__ volatile ("" : : : "memory");
        screenX[i] = H(slotBase, 0x8);
        screenY[i] = H(slotBase, 0xA);
    }

    for (i = 0; i < 20; i++) {
        unsigned int bufOff = (unsigned int)B(Scratchpad, 0x29D) * 0x3AC0u;
        unsigned char* prim = (unsigned char*)W(Scratchpad, 0x190) + bufOff + slotOffset + (unsigned int)(i * 0x20);
        int v0Idx = S(D_800CA774, i * 6);
        int v1Idx = S(D_800CA778, i * 6 - 2);
        int v2Idx = S(D_800CA778, i * 6);
        int otZ;

        /* UNCLEAR: GTE RTPS transforms D_800E6D90 + v0Idx*8, writes SXY2 ($14) to
         * prim + 8, and reads SZ3 ($19) >> 2 into Scratchpad+0xEC. */
        (void)v0Idx;
        __asm__ volatile ("" : : : "memory");

        H(prim, 0x10) = screenX[v1Idx];
        H(prim, 0x12) = screenY[v1Idx];
        H(prim, 0x18) = screenX[v2Idx];
        H(prim, 0x1A) = screenY[v2Idx];

        otZ = SW(Scratchpad, 0x0EC);
        if (otZ > 0) {
            unsigned int otShift = 14u - W(Scratchpad, 0x000);
            unsigned int otIndex = (unsigned int)(otZ >> otShift);
            unsigned char* otEntry = D_800F3568 + ((unsigned int)B(Scratchpad, 0x29D) << 14) + (otIndex << 2);
            func_800ADC04(otEntry, prim);
        }
    }
}
