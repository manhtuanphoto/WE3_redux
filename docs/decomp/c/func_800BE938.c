/*
 * func_800BE938
 *
 * Purpose:
 *   Initializes the sound/SPU subsystem: resets SPU transfer/driver state,
 *   clears sound state tables (D_8010A418, D_800E81E0, D_8010A3F0), clamps the
 *   requested voice count to 24 (0x18), initializes up to 24 voice channel
 *   records (stride 0x36 = 54 bytes starting at D_800E78D8) and SPU voice
 *   attributes via func_800C44A8/func_800BFD18, clears mixer/playback globals,
 *   and calls func_800BE4A8.
 *
 * Arguments:
 *   arg0: (signed char) requested number of SPU voice channels (clamped to <= 24)
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Globals:
 *     D_800EF980, D_8010CDD8, D_800E75F8..D_800E7600, D_800E764C,
 *     D_800F0F70 (s16 = 0), D_800F106C (s16 = 0x80), D_8010CE6C (s16 = 0),
 *     D_8010A5A0 (u8 = 0), D_8010A418 (192 u16s = 0), D_800E81E0 (24 u8s = 0),
 *     D_8010A3F0 (16 u8s = 0), D_800FB578 (s8 voice count),
 *     D_800E78D8..D_800E790A (24 voice structs, stride 0x36),
 *     D_8010A3E2 (s16 current voice index), D_800E81B8 (12 bytes mixer state)
 *
 * Magic numbers:
 *   0x18   = 24 maximum PS1 SPU hardware voices
 *   0x36   = 54 bytes per sound channel entry in D_800E78D8
 *   0x60093 = SpuVoiceAttr mask (pitch/volume/ADSR fields)
 *   0x3FFF = max SPU master/CD volume
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800E7220[];
extern unsigned char D_800EF980[];
extern unsigned char D_8010A418[];
extern unsigned char D_800E81E0[];
extern unsigned char D_8010CDD8[];
extern unsigned char D_8010A3F0[];
extern unsigned char D_800FB578[];
extern unsigned char D_800E78D8[];
extern unsigned char D_8010A3E2[];
extern unsigned char D_800E81B8[];
extern unsigned char D_800E75F8[];
extern unsigned char D_800E75FA[];
extern unsigned char D_8010CE6C[];
extern unsigned char D_800E75FC[];
extern unsigned char D_800E75FE[];
extern unsigned char D_800E7600[];
extern unsigned char D_800E764C[];
extern unsigned char D_8010A5A0[];
extern unsigned char D_800F0F70[];
extern unsigned char D_800F106C[];

extern void func_800C3F88(int);
extern void func_800C1EA8(int, unsigned char*);
extern void func_800C44A8(unsigned char*);
extern void func_800BFD18(int);
extern void func_800BE4A8(void);

void func_800BE938(int arg0) {
    unsigned char attr[0x40];
    int i;
    signed char count;

    func_800C3F88(0);
    S(D_800EF980, 0) = 0;
    func_800C1EA8(0x20, D_800E7220);

    for (i = 0; (unsigned int)(i & 0xFFFF) < 0xC0U; i++) {
        H(D_8010A418, (i & 0xFFFF) * 2) = 0;
    }
    for (i = 0; (unsigned int)(i & 0xFFFF) < 0x18U; i++) {
        B(D_800E81E0, i & 0xFFFF) = 0;
    }
    S(D_8010CDD8, 0) = 0;
    for (i = 0; (unsigned int)(i & 0xFFFF) < 0x10U; i++) {
        B(D_8010A3F0, i & 0xFFFF) = 0;
    }

    count = (signed char)arg0;
    if ((unsigned int)(int)count >= 0x18U) {
        B(D_800FB578, 0) = 0x18;
    } else {
        B(D_800FB578, 0) = (unsigned char)count;
    }

    W(attr, 0x04) = 0x60093;
    S(attr, 0x14) = 0x1000;
    W(attr, 0x1C) = 0x1000;
    H(attr, 0x3A) = 0x80FF;
    S(attr, 0x08) = 0;
    S(attr, 0x0A) = 0;
    S(attr, 0x3C) = 0x4000;

    i = 0;
    if ((signed char)B(D_800FB578, 0) > 0) {
        do {
            unsigned int idx = (unsigned int)(i & 0xFFFF);
            unsigned char* ch = D_800E78D8 + idx * 0x36;

            S(ch, 0x02) = 0x18;
            S(ch, 0x00) = 0xFF;
            B(ch, 0x1D) = 0;
            S(ch, 0x04) = 0;
            S(ch, 0x06) = 0;
            S(ch, 0x10) = -1;
            S(ch, 0x12) = 0;
            S(ch, 0x14) = 0;
            S(ch, 0x16) = 0xFF;
            S(ch, 0x08) = 0;
            S(ch, 0x0C) = 0;
            B(ch, 0x0A) = 0x40;
            S(ch, 0x1E) = 0;
            S(ch, 0x20) = 0;
            S(ch, 0x22) = 0;
            S(ch, 0x24) = 0;
            S(ch, 0x2A) = 0;
            S(ch, 0x2C) = 0;
            S(ch, 0x2E) = 0;
            S(ch, 0x30) = 0;
            S(ch, 0x32) = 0;
            S(ch, 0x26) = 0;

            W(attr, 0x00) = 1U << idx;
            func_800C44A8(attr);
            S(D_8010A3E2, 0) = (signed short)i;
            func_800BFD18(1);
            i++;
        } while ((int)(i & 0xFFFF) < (int)(signed char)B(D_800FB578, 0));
    }

    W(D_800E81B8, 0x0) = 0;
    S(D_800E81B8, 0x8) = 0x3FFF;
    S(D_800E81B8, 0xA) = 0x3FFF;
    W(D_800E81B8, 0x4) = 0;
    S(D_800E75F8, 0) = 0;
    S(D_800E75FA, 0) = 0;
    S(D_8010CE6C, 0) = 0;
    S(D_800E75FC, 0) = 0;
    S(D_800E75FE, 0) = 0;
    S(D_800E7600, 0) = 0;
    S(D_800E764C, 0) = 0;
    B(D_8010A5A0, 0) = 0;
    S(D_800F0F70, 0) = 0;
    S(D_800F106C, 0) = 0x80;
    func_800BE4A8();
}
