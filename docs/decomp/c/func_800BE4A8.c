/*
 * func_800BE4A8
 *
 * Purpose:
 *   Per-frame SPU/sound driver update: polls active sound channels in a 16-slot
 *   ring buffer, releases finished channels, runs per-channel callbacks for the
 *   24 voices, flushes pending SpuVoiceAttr attribute updates, and submits SPU
 *   key-on / key-off / pitch/reverb bitmasks before clearing the staging masks.
 *
 * Arguments:
 *   void
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Globals:
 *     D_8010A5B8 (u32)  16-slot ring buffer index (0..15) (R/W)
 *     D_8010CE28 (u32[16]) ring buffer of idle-channel bitmasks (R/W)
 *     D_800FB578 (s8)   active channel count (R)
 *     D_800E78DE (u8[]) channel table, stride 0x36 = 54 bytes (+0x00 u16 status,
 *                       +0x17 s8 state at D_800E78F5, +0x18 s16 cb1 at D_800E78F6,
 *                       +0x24 s16 cb2 at D_800E7902) (R/W)
 *     D_8010A5A0 (s8)   auto-release disable flag (R)
 *     D_8010A598 (fn*)  callback 1 pointer (R)
 *     D_800FF730 (fn*)  callback 2 pointer (R)
 *     D_800E81E0 (u8[24]) per-voice dirty attribute bitmask (R/W)
 *     D_8010A418 (u8[]) 24 x 16-byte voice attribute staging structs (R)
 *     D_8010CE6C/6E, D_800E75F8/FA, D_800E75FC/FE, D_800E7600/4C (u16)
 *                       24-bit voice bitmasks (low 16 + high 8) (R/W)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x18 = 24 PS1 SPU hardware voices
 *   0x36 = 54-byte sound channel entry stride
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800E75F8[];
extern unsigned char D_800E75FA[];
extern unsigned char D_800E75FC[];
extern unsigned char D_800E75FE[];
extern unsigned char D_800E7600[];
extern unsigned char D_800E764C[];
extern unsigned char D_800E78DE[];
extern unsigned char D_800E78F5[];
extern unsigned char D_800E78F6[];
extern unsigned char D_800E7902[];
extern unsigned char D_800E81E0[];
extern unsigned char D_800FB578[];
extern unsigned char D_800FF730[];
extern unsigned char D_8010A418[];
extern unsigned char D_8010A598[];
extern unsigned char D_8010A5A0[];
extern unsigned char D_8010A5B8[];
extern unsigned char D_8010CE28[];
extern unsigned char D_8010CE6C[];
extern unsigned char D_8010CE6E[];

extern void func_800C4DF8(int, unsigned char*);
extern void func_800C2AE8(int, unsigned int);
extern void func_800C44A8(unsigned char*);
extern void func_800C3B98(int, unsigned int);
extern void func_800C39B8(int, unsigned int);

void func_800BE4A8(void) {
    unsigned int ringIdx;
    int chanCount;
    int i;
    unsigned int idleMask;
    unsigned char spuAttr[96]; /* 0x10($sp)..0x6F($sp) */

    ringIdx = (W(D_8010A5B8, 0) + 1) & 0xF;
    W(D_8010A5B8, 0) = ringIdx;
    W(D_8010CE28, ringIdx * 4) = 0;

    chanCount = *(signed char*)D_800FB578;
    for (i = 0; i < chanCount; i++) {
        func_800C4DF8(i, D_800E78DE + i * 0x36);
        if (H(D_800E78DE, i * 0x36) == 0) {
            W(D_8010CE28, W(D_8010A5B8, 0) * 4) |= (1U << i);
        }
    }

    if (*(signed char*)D_8010A5A0 == 0) {
        idleMask = 0xFFFFFFFFU;
        for (i = 0; i < 15; i++) {
            idleMask &= W(D_8010CE28, i * 4);
        }

        chanCount = *(signed char*)D_800FB578;
        for (i = 0; i < chanCount; i++) {
            unsigned int bit = 1U << i;
            if ((idleMask & bit) != 0) {
                if (*(signed char*)(D_800E78F5 + i * 0x36) == 2) {
                    unsigned int highBits = 0;
                    int lowBits = (int)bit;
                    if (i >= 16) {
                        lowBits = 0;
                        highBits = (1U << (i - 16)) & 0xFF;
                    }
                    lowBits = (lowBits << 16) >> 16;
                    func_800C2AE8(0, (highBits << 16) | (unsigned int)lowBits);
                }
                B(D_800E78F5, i * 0x36) = 0;
            }
        }
    }

    H(D_800E75F8, 0) &= (unsigned short)~H(D_8010CE6C, 0);
    H(D_800E75FA, 0) &= (unsigned short)~H(D_8010CE6E, 0);

    for (i = 0; i < 24; i++) {
        if (S(D_800E78F6, i * 0x36) != 0) {
            ((void (*)(int))W(D_8010A598, 0))(i);
        }
        if (S(D_800E7902, i * 0x36) != 0) {
            ((void (*)(int))W(D_800FF730, 0))(i);
        }
    }

    for (i = 0; i < 24; i++) {
        unsigned int attrFlags = 0;
        unsigned char dirty = B(D_800E81E0, i);
        unsigned char* entry = D_8010A418 + i * 0x10;

        W(spuAttr, 0x00) = 1U << i;
        if (dirty & 0x01) {
            attrFlags = 3;
            H(spuAttr, 0x08) = H(entry, 0x00);
            H(spuAttr, 0x0A) = H(entry, 0x02);
        }
        if (dirty & 0x04) {
            attrFlags |= 0x10;
            H(spuAttr, 0x14) = H(entry, 0x04);
        }
        if (dirty & 0x08) {
            attrFlags |= 0x80;
            W(spuAttr, 0x1C) = (unsigned int)H(entry, 0x06) << 3;
        }
        if (dirty & 0x10) {
            attrFlags |= 0x60000;
            H(spuAttr, 0x3A) = H(entry, 0x08);
            H(spuAttr, 0x3C) = H(entry, 0x0A);
        }
        W(spuAttr, 0x04) = attrFlags;
        if (attrFlags != 0) {
            func_800C44A8(spuAttr);
        }
        B(D_800E81E0, i) = 0;
    }

    func_800C3B98(0, ((unsigned int)B(D_8010CE6E, 0) << 16) | H(D_8010CE6C, 0));
    func_800C3B98(1, ((unsigned int)B(D_800E75FA, 0) << 16) | H(D_800E75F8, 0));
    func_800C39B8(8, ((unsigned int)B(D_800E75FE, 0) << 16) | H(D_800E75FC, 0));
    func_800C2AE8(8, ((unsigned int)B(D_800E764C, 0) << 16) | H(D_800E7600, 0));

    H(D_8010CE6C, 0) = 0;
    H(D_8010CE6E, 0) = 0;
    H(D_800E75F8, 0) = 0;
    H(D_800E75FA, 0) = 0;
    H(D_800E7600, 0) = 0;
    H(D_800E764C, 0) = 0;
}
