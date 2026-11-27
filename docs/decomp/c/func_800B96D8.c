/*
 * func_800B96D8 (plus contiguous functions 0x800B9764, 0x800B97BC, 0x800B9840)
 *
 * Purpose:
 *   STR video / CD-ROM ring-buffer management helpers (paired with func_800B9A28):
 *     - func_800B96D8: Completes/commits current ring-buffer frame slot, updates
 *       D_800E71F8 / D_800E71FC / D_8010C1C4, invokes frame callback D_800EF8AC,
 *       and clears D_800FF680.
 *     - sub_800B9764: Seeks/starts stream at frame offset a0 if D_800E81D8 == 0,
 *       returning D_800E71FC (or -1 if D_800E81D8 != 0).
 *     - sub_800B97BC: Initializes STR ring-buffer streaming state and callbacks.
 *     - sub_800B9840: Frees a ring-buffer frame's 2016-byte (0x1F8 words) sector
 *       range given a payload pointer inside D_8010CD8C + (D_8010CE68 << 5).
 *
 * Global fields read/written:
 *   D_8010CD8C (u8*)  ring-buffer 32-byte sector header array base
 *   D_8010CE68 (u32)  ring-buffer capacity in sectors (payload follows header array)
 *   D_8010C1C0 (u32)  write sector index
 *   D_8010C1C4 (u32)  frame-start sector index
 *   D_8010C1CC (u32)  last freed sector index
 *   D_800E71F8 (u8[4]) frame header/location bytes (+0x1C..+0x1F of slot)
 *   D_800E71FC (u32)  frame metadata word (+0x08 of slot)
 *   D_800EF8AC (void (*)(void)) frame-ready callback
 *   D_800EF8B4 (u32)  overflow/limit callback
 *   D_800FF680 (u32)  frame-complete flag
 *   D_800E81D8 (u32)  skip-header-drain flag
 *   D_8010CD4C, D_800E81D4, D_800F3560, D_800F0F68, D_800E81CC, D_800E7654
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned int  D_8010C1C4;
extern unsigned int  D_8010C1C0;
extern unsigned int  D_8010C1CC;
extern unsigned int  D_8010CE68;
extern unsigned char *D_8010CD8C;
extern unsigned char D_800E71F8[];
extern unsigned int  D_800E71FC;
extern void (*D_800EF8AC)(void);
extern unsigned int  D_800FF680;
extern unsigned int  D_800E81D8;
extern unsigned int  D_8010CD4C;
extern unsigned int  D_800E81D4;
extern unsigned int  D_800F3560;
extern unsigned int  D_800F0F68;
extern unsigned short D_800E81CC;
extern unsigned int  D_800E7654;
extern unsigned int  D_800EF8B4;

extern int  func_800B7598(unsigned char *loc);
extern void func_800B7494(int sector, unsigned int arg1);
extern void func_800B9A08(unsigned int arg0);

void func_800B96D8(void)
{
    unsigned char *slot = D_8010CD8C + (D_8010C1C4 << 5);
    int i;

    S(slot, 0) = 2;
    for (i = 0; i < 4; i++) {
        D_800E71F8[i] = B(slot, 0x1C + i);
    }
    D_800E71FC = W(slot, 8);
    D_8010C1C4 = D_8010C1C0;

    if (D_800EF8AC != 0) {
        D_800EF8AC();
    }

    D_800FF680 = 0;
}

signed int sub_800B9764(unsigned int a0)
{
    if (D_800E81D8 == 0) {
        int pos = func_800B7598(D_800E71F8);
        func_800B7494(pos + 1, a0);
        return (signed int)D_800E71FC;
    }
    return -1;
}

void sub_800B97BC(unsigned int a0, unsigned int a1, unsigned int a2,
                  void (*a3)(void), unsigned int arg4)
{
    (void)a1;
    (void)a2;
    func_800B9A08(1);
    D_8010CD4C = 0;
    D_800EF8AC = a3;
    D_800E81D4 = a0 & 1U;
    D_800F3560 = 0;
    D_800F0F68 = 0;
    D_800E81CC = 0;
    D_800E7654 = 0;
    D_800EF8B4 = arg4;
}

signed int sub_800B9840(unsigned char *payload_ptr)
{
    unsigned char *hdr_base = D_8010CD8C;
    unsigned char *payload_base = hdr_base + (D_8010CE68 << 5);
    signed int idx = (signed int)(payload_ptr - payload_base) / 2016;
    unsigned char *slot = hdr_base + (idx << 5);

    if (S(slot, 0) == 4) {
        signed short count = S(slot, 6);
        signed int i = 0;
        if (count > 0) {
            for (i = 0; i < count; i++) {
                S(D_8010CD8C + ((i + idx) << 5), 0) = 0;
            }
        }
        D_8010C1CC = (unsigned int)(i + idx);
        return 0;
    }

    return 1;
}
