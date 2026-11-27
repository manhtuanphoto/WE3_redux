/*
 * func_800B0E70
 *
 * Purpose:
 *   Initiates a VRAM / image transfer (PsyQ LoadImage-style routine). Logs the
 *   source RECT via func_800AE59C, sets a timeout counter, polls hardware/DMA
 *   status registers (D_800CEBA8 / D_800CEB9C) while running func_800B0AB4,
 *   registers callback func_800B10B0, validates non-zero width/height, and
 *   submits the transfer command via function pointer *(D_800CEABC + 0x18).
 *
 * Arguments:
 *   rect (short *): pointer to 4-short RECT { x, y, w, h }
 *   src_lo (unsigned int): packed low 16-bit parameter (e.g., source/transfer arg)
 *   src_hi (unsigned int): packed high 16-bit parameter
 *
 * Return value:
 *   0 on success; -1 on timeout/abort from func_800B0AB4 or if rect->w == 0 or rect->h == 0.
 *
 * Global fields read:
 *   D_80014F6C (format/debug string for func_800AE59C)
 *   D_800CEBA8 (volatile unsigned int *): status register pointer (bit 24 = 0x01000000)
 *   D_800CEB9C (volatile unsigned int *): status register pointer (bit 26 = 0x04000000)
 *   D_800CEABC (unsigned char *): vtable/jump-table pointer (+0x18 callback)
 *
 * Global fields written:
 *   D_800CEBD0 (unsigned int): set to func_800BA518(-1) + 0xF0
 *   D_800CEBD4 (unsigned int): cleared to 0
 *   D_800CEB64 (unsigned int[3]): populated with rect words and packed (src_hi, src_lo)
 */

#define W(p, o)  (*(unsigned int *)((unsigned char *)(p) + (o)))
#define SH(p, o) (*(signed short *)((unsigned char *)(p) + (o)))

extern unsigned char D_80014F6C[];
extern volatile unsigned int *D_800CEBA8;
extern volatile unsigned int *D_800CEB9C;
extern unsigned int D_800CEBD0;
extern unsigned int D_800CEBD4;
extern unsigned char *D_800CEABC;
extern unsigned int D_800CEB64[];

extern void func_800AE59C(unsigned char *fmt, short *rect);
extern unsigned int func_800BA518(int arg);
extern int func_800B0AB4(void);
extern void func_800BA798(int irq_id, void (*cb)(void));
extern void func_800B10B0(void);

int func_800B0E70(short *rect, unsigned int src_lo, unsigned int src_hi)
{
    unsigned int packed;

    func_800AE59C(D_80014F6C, rect);

    D_800CEBD0 = func_800BA518(-1) + 0xF0;
    D_800CEBD4 = 0;

    while ((*D_800CEBA8 & 0x01000000u) != 0 || (*D_800CEB9C & 0x04000000u) == 0) {
        if (func_800B0AB4() != 0) {
            return -1;
        }
    }

    func_800BA798(2, func_800B10B0);

    if (SH(rect, 0x04) == 0) {
        return -1;
    }
    if (SH(rect, 0x06) == 0) {
        return -1;
    }

    packed = (src_hi << 16) | (src_lo & 0xFFFFu);
    D_800CEB64[1] = packed;
    D_800CEB64[0] = W(rect, 0x00);
    D_800CEB64[2] = W(rect, 0x04);

    /* Passes &D_800CEB64 - 8 (D_800CEB5C) to the transfer handler at D_800CEABC + 0x18 */
    ((void (*)(void *))W(D_800CEABC, 0x18))((unsigned char *)D_800CEB64 - 8);

    return 0;
}
