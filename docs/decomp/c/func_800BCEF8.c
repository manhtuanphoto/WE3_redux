/*
 * func_800BCEF8 (plus two unlabeled adjacent functions in the same .s unit)
 *
 * Purpose:
 *   Sound/SPU/SEQ parameter wrappers around D_8010C2A8 (table of pointers to
 *   0xB0-byte sound/sequence channel structures) and func_800BF364 / func_800C02B8.
 *   1) func_800BCEF8: Unpacks packed 16-bit/8-bit fields from a0 and a1 and
 *      forwards 6 arguments to func_800BF364.
 *   2) func_800BCF44 (unlabeled at +0x4C): Indexes D_8010C2A8[(s16)a0] + ((s16)a1 * 0xB0).
 *      If *(s32 *)(entry + 0x98) == 1, calls func_800C02B8; otherwise writes
 *      halfwords a2, a3 into entry+0x58, entry+0x5A.
 *   3) func_800BCFD8 (unlabeled at +0xE0): Indexes D_8010C2A8[(s16)a0].
 *      If *(s32 *)(entry + 0x98) == 1, calls func_800C02B8; otherwise writes
 *      halfwords a1, a2 into entry+0x58, entry+0x5A.
 *
 * Global fields read:
 *   D_8010C2A8 (unsigned char *[]): table of sound/sequence struct pointers
 *
 * Struct fields accessed (relative to entry):
 *   +0x58 (s16), +0x5A (s16): volume/pan or pitch parameters
 *   +0x98 (s32): active/mode flag (1 triggers func_800C02B8)
 *
 * Notes:
 *   UNCLEAR: Exact boundary symbols and names for the two adjacent functions at
 *   +0x4C and +0xE0 inside func_800BCEF8.s (emitted without separate glabels).
 */

#define H(p, o)  (*(unsigned short *)((unsigned char *)(p) + (o)))
#define SH(p, o) (*(signed short *)((unsigned char *)(p) + (o)))
#define SW(p, o) (*(signed int *)((unsigned char *)(p) + (o)))

extern unsigned char *D_8010C2A8[];

extern int func_800BF364(int a0, unsigned int a1, unsigned int a2, unsigned int a3,
                         unsigned int a4, unsigned int a5);
extern int func_800C02B8(int a0, unsigned int a1, unsigned int a2, unsigned int a3);

int func_800BCEF8(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3)
{
    int p0 = ((signed int)(a0 << 8)) >> 16;
    unsigned int p1 = a0 & 0xFFu;
    unsigned int p2 = (a1 >> 8) & 0xFFFFu;
    unsigned int p3 = a1 & 0xFFu;
    unsigned int p4 = a2 & 0xFFFFu;
    unsigned int p5 = a3 & 0xFFFFu;

    return func_800BF364(p0, p1, p2, p3, p4, p5);
}

/* UNCLEAR: unlabeled adjacent function at +0x4C in func_800BCEF8.s */
void func_800BCF44(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3)
{
    signed int idx0 = (signed short)a0;
    signed int idx1 = (signed short)a1;
    unsigned char *entry = D_8010C2A8[idx0] + (idx1 * 0xB0);

    if (SW(entry, 0x98) == 1) {
        signed int packed_id = (signed short)(a0 | (a1 << 8));
        func_800C02B8(packed_id, a2 & 0xFFFFu, a3 & 0xFFFFu, 1u);
    } else {
        H(entry, 0x58) = (unsigned short)a2;
        H(entry, 0x5A) = (unsigned short)a3;
    }
}

/* UNCLEAR: unlabeled adjacent function at +0xE0 in func_800BCEF8.s */
void func_800BCFD8(unsigned int a0, unsigned int a1, unsigned int a2)
{
    signed int idx = (signed short)a0;
    unsigned char *entry = D_8010C2A8[idx];

    if (SW(entry, 0x98) == 1) {
        func_800C02B8(idx, a1 & 0xFFFFu, a2 & 0xFFFFu, 1u);
    } else {
        H(entry, 0x58) = (unsigned short)a1;
        H(entry, 0x5A) = (unsigned short)a2;
    }
}
