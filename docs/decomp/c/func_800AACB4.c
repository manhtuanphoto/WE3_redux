/*
 * func_800AACB4
 *
 * Purpose:
 *   Configures a 20-entry display/UI sequence in D_800CE26C (starting at slot
 *   index 2) from two source descriptors in D_800CE1B4 indexed by (arg0, arg1),
 *   balancing slots between arg0 and arg1 (10+10 if counts are equal, 15+5 if
 *   arg0's count < arg1's count, or 5+15 if arg0's count > arg1's count), stores
 *   (arg0 - 1) and (arg1 - 1) into D_800E6BC4/D_800E6BC6, and plays SFX 0x17A1.
 *
 * Arguments:
 *   arg0: (u8) first descriptor index into D_800CE1B4 (1-based ID)
 *   arg1: (u8) second descriptor index into D_800CE1B4 (1-based ID)
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Globals:
 *     D_800E6BC4 (s16) = (arg0 & 0xFF) - 1 (W)
 *     D_800E6BC6 (s16) = (arg1 & 0xFF) - 1 (W)
 *     D_800CE1B4 (4-byte entries: +0 u16 baseIdx, +2 s16 count) (R)
 *     D_800CE26C (4-byte entries: +0 u16 val, +2 u8 byte) (R/W)
 *
 * Magic numbers:
 *   0x17A1 = sound effect ID passed to func_800AB620
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800E6BC4[];
extern unsigned char D_800E6BC6[];
extern unsigned char D_800CE1B4[];
extern unsigned char D_800CE26C[];

extern void func_800AB620(unsigned int sfxId);

void func_800AACB4(unsigned int arg0, unsigned int arg1) {
    unsigned int idx0 = arg0 & 0xFFU;
    unsigned int idx1 = arg1 & 0xFFU;
    unsigned short base0;
    unsigned short base1;
    signed short count0;
    signed short count1;
    int i;

    S(D_800E6BC4, 0) = (signed short)(idx0 + 0xFFFFU);
    S(D_800E6BC6, 0) = (signed short)(idx1 + 0xFFFFU);

    base0 = H(D_800CE1B4, idx0 * 4);
    count0 = S(D_800CE1B4, idx0 * 4 + 2);
    count1 = S(D_800CE1B4, idx1 * 4 + 2);
    base1 = H(D_800CE1B4, idx1 * 4);

    if (count0 == count1) {
        int off0 = (signed short)base0;
        int off1 = (signed short)base1;
        for (i = 0; (signed short)i < 10; i++) {
            int k = (signed short)i;
            H(D_800CE26C, (k + 2) * 4) = H(D_800CE26C, (off0 + k) * 4);
            B(D_800CE26C, (k + 2) * 4 + 2) = B(D_800CE26C, (off0 + k) * 4 + 2);
            H(D_800CE26C, (k + 12) * 4) = H(D_800CE26C, (off1 + k) * 4);
            B(D_800CE26C, (k + 12) * 4 + 2) = B(D_800CE26C, (off1 + k) * 4 + 2);
        }
    } else if (count0 < count1) {
        int off0 = (signed short)base0;
        int off1 = (signed short)base1;
        for (i = 0; (signed short)i < 15; i++) {
            int k = (signed short)i;
            H(D_800CE26C, (k + 2) * 4) = H(D_800CE26C, (off0 + k + 5) * 4);
            B(D_800CE26C, (k + 2) * 4 + 2) = B(D_800CE26C, (off0 + k + 5) * 4 + 2);
        }
        for (i = 0; (signed short)i < 5; i++) {
            int k = (signed short)i;
            H(D_800CE26C, (k + 17) * 4) = H(D_800CE26C, (off1 + k) * 4);
            B(D_800CE26C, (k + 17) * 4 + 2) = B(D_800CE26C, (off1 + k) * 4 + 2);
        }
    } else {
        int off0 = (signed short)base0;
        int off1 = (signed short)base1;
        for (i = 0; (signed short)i < 5; i++) {
            int k = (signed short)i;
            H(D_800CE26C, (k + 2) * 4) = H(D_800CE26C, (off0 + k) * 4);
            B(D_800CE26C, (k + 2) * 4 + 2) = B(D_800CE26C, (off0 + k) * 4 + 2);
        }
        for (i = 0; (signed short)i < 15; i++) {
            int k = (signed short)i;
            H(D_800CE26C, (k + 7) * 4) = H(D_800CE26C, (off1 + k + 5) * 4);
            B(D_800CE26C, (k + 7) * 4 + 2) = B(D_800CE26C, (off1 + k + 5) * 4 + 2);
        }
    }

    func_800AB620(0x17A1);
}
