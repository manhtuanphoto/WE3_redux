/*
 * func_800C0C58
 *
 * Purpose:
 *   Opens and parses a PS1 VAB soundbank header (PsyQ SsVabOpenHead*-style),
 *   allocating a VAB slot (0..15), indexing programs/tones and VAG sizes,
 *   allocating SPU RAM (if arg2 == 0) and computing SPU byte>>3 addresses for
 *   each VAG waveform.
 *
 * Arguments:
 *   vabData (u8 *): pointer to VAB header data in main RAM ("pBAV" magic)
 *   vabId   (s16):  requested VAB slot (0..15) or -1 to auto-allocate
 *   arg2    (s16):  0 to allocate SPU memory via func_800C1F08, non-zero to use spuAddr
 *   spuAddr (u32):  explicit SPU RAM address when arg2 != 0
 *
 * Return value:
 *   s16: allocated VAB slot ID (0..15) on success, or -1 on error.
 *
 * Global fields read/written:
 *   D_8010A3F0[16] (u8)  VAB slot state (0=free, 1=opening, 2=open)
 *   D_8010CDD8     (u16) active VAB count
 *   D_800EF940[16] (u32) VAB header pointers
 *   D_800EF8B8[16] (u32) VAB program attribute table pointers
 *   D_800EF988[16] (u32) VAB tone attribute table pointers
 *   D_8010CDE0[16] (u32) VAB SPU base addresses
 *   D_8010CD98[16] (u32) total VAG data size in bytes
 *   D_800F106C     (s16) max programs (0x40 = 64 or 0x80 = 128 for VAB v5+)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char D_8010A3F0[];
extern unsigned short D_8010CDD8;
extern unsigned int D_800EF940[];
extern unsigned int D_800EF8B8[];
extern unsigned int D_800EF988[];
extern unsigned int D_8010CDE0[];
extern unsigned int D_8010CD98[];
extern signed short D_800F106C;

extern int func_800C3FB0(void);
extern void func_800C3F88(int state);
extern int func_800C1F08(unsigned int size);

int func_800C0C58(unsigned char *vabData, signed short vabId, signed short arg2, unsigned int spuAddr)
{
    unsigned int vagSizes[256];
    signed short slot = 0x10;
    signed short reqId = vabId;
    unsigned char *vabBody;
    unsigned char *tonePtr;
    unsigned char *vagSizeTable;
    unsigned int headerWord;
    unsigned int numVags;
    unsigned int totalSize;
    unsigned int spuBase;
    int i;
    int progCount;

    if (func_800C3FB0() == 1) {
        return -1;
    }

    func_800C3F88(1);

    if (reqId >= 0x10) {
        func_800C3F88(0);
        return -1;
    }

    if (reqId == -1) {
        for (i = 0; i < 0x10; i++) {
            if (D_8010A3F0[i] == 0) {
                D_8010A3F0[i] = 1;
                D_8010CDD8++;
                slot = (signed short)i;
                break;
            }
        }
    } else if (D_8010A3F0[reqId] == 0) {
        D_8010A3F0[reqId] = 1;
        D_8010CDD8++;
        slot = reqId;
    }

    if (slot >= 0x10) {
        func_800C3F88(0);
        return -1;
    }

    D_800EF940[slot] = (unsigned int)vabData;
    headerWord = W(vabData, 0);
    vabBody = vabData + 0x20;

    if ((headerWord >> 8) != 0x564142U) {
        D_8010A3F0[slot] = 0;
        func_800C3F88(0);
        D_8010CDD8--;
        return -1;
    }

    if ((headerWord & 0xFF) == 0x70 && SW(vabData, 4) >= 5) {
        D_800F106C = 0x80;
    } else {
        D_800F106C = 0x40;
    }

    if (D_800F106C < (signed int)H(vabData, 0x12)) {
        D_8010A3F0[slot] = 0;
        func_800C3F88(0);
        D_8010CDD8--;
        return -1;
    }

    D_800EF8B8[slot] = (unsigned int)vabBody;
    tonePtr = vabBody + (D_800F106C << 4);

    progCount = 0;
    for (i = 0; i < D_800F106C; i++) {
        unsigned char *progEntry = vabBody + i * 0x10;
        if (B(progEntry, 0) != 0) {
            SW(progEntry, 8) = progCount;
            progCount++;
        }
    }

    totalSize = 0;
    D_800EF988[slot] = (unsigned int)tonePtr;
    vagSizeTable = tonePtr + ((unsigned int)H(vabData, 0x12) << 9);
    numVags = B(vabData, 0x16);

    for (i = 0; i < 0x100; i++) {
        if (i <= (int)numVags) {
            unsigned int sz = (SW(vabData, 4) < 5)
                ? ((unsigned int)H(vagSizeTable, 0) << 2)
                : ((unsigned int)H(vagSizeTable, 0) << 3);
            vagSizes[i] = sz;
            totalSize += vagSizes[i];
        }
        vagSizeTable += 2;
    }

    if (arg2 == 0) {
        spuBase = (unsigned int)func_800C1F08(totalSize);
        if ((int)spuBase == -1) {
            D_8010A3F0[slot] = 0;
            func_800C3F88(0);
            D_8010CDD8--;
            return -1;
        }
    } else {
        spuBase = spuAddr;
    }

    if (spuBase + totalSize > 0x80000U) {
        D_8010A3F0[slot] = 0;
        func_800C3F88(0);
        D_8010CDD8--;
        return -1;
    }

    totalSize = 0;
    D_8010CDE0[slot] = spuBase;
    for (i = 0; i <= (int)numVags; i++) {
        unsigned char *vagEntry = vabBody + (i / 2) * 0x10;
        totalSize += vagSizes[i];
        if ((i & 1) == 0) {
            H(vagEntry, 0xC) = (unsigned short)((spuBase + totalSize) >> 3);
        } else {
            H(vagEntry, 0xE) = (unsigned short)((spuBase + totalSize) >> 3);
        }
    }

    D_8010CD98[slot] = totalSize;
    D_8010A3F0[slot] = 2;
    return (int)slot;
}
