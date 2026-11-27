/*
 * func_800C4E54 (and 10 adjacent unsplit GPU/interrupt helper functions:
 *   func_800C4E78, func_800C4EAC, func_800C4F38, func_800C4FD0, func_800C4FDC,
 *   func_800C5058, func_800C5078, func_800C50B4, func_800C50F0, func_800C5114)
 *
 * Purpose:
 *   Low-level PS1 GPU / VSync / DrawSync wrapper routines (callback registration,
 *   graph reset, 256-byte GPU state save/restore across D_800D5900/D_800D5940/D_800D5984,
 *   display mask/mode flag updates, and GPU status register bit queries).
 *
 * Arguments:
 *   See individual sub-functions below.
 *
 * Return value:
 *   See individual sub-functions below.
 *
 * Reads / Writes:
 *   D_800D58FC, D_800D5900 (64B), D_800D5940 (64B), D_800D5980, D_800D5984 (128B):
 *     global GPU/display environment buffers.
 *
 * Magic numbers:
 *   0x08000000 / 0x02000000: GPU control/tag bits toggled in func_800C4FDC.
 *   Bits 29 and 24 of func_800C546C() return value queried in func_800C5078 / func_800C50B4.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern void func_800BA798(int index, void *cb);
extern void func_800BA738(void);
extern int func_800C5138(int mode);
extern void func_800C5228(unsigned char *buf, unsigned int size);
extern int func_800C52B8(void);
extern int func_800C5344(void);
extern int func_800C53D8(void);
extern unsigned int func_800C546C(void);

extern unsigned char D_800D58FC[];
extern unsigned char D_800D5900[];
extern unsigned char D_800D5940[];
extern unsigned char D_800D5980[];
extern unsigned char D_800D5984[];

/* 0x800C4E54 */
void func_800C4E54(void *cb) {
    func_800BA798(3, cb);
}

/* 0x800C4E78 */
int func_800C4E78(int mode) {
    if (mode == 0) {
        func_800BA738();
    }
    return func_800C5138(mode);
}

/* 0x800C4EAC */
unsigned char *func_800C4EAC(unsigned char *dst) {
    int i;
    for (i = 0; i < 16; i++) {
        W(dst, i * 4) = W(D_800D5900, i * 4);
    }
    for (i = 0; i < 16; i++) {
        W(dst, 0x40 + i * 4) = W(D_800D5940, i * 4);
    }
    for (i = 0; i < 32; i++) {
        W(dst, 0x80 + i * 4) = W(D_800D5984, i * 4);
    }
    return dst;
}

/* 0x800C4F38 */
unsigned char *func_800C4F38(unsigned char *src) {
    int i;
    for (i = 0; i < 16; i++) {
        W(D_800D5900, i * 4) = W(src, i * 4);
    }
    for (i = 0; i < 16; i++) {
        W(D_800D5940, i * 4) = W(src, 0x40 + i * 4);
    }
    func_800C5228(D_800D58FC, 0x20);
    func_800C5228(D_800D5980, 0x20);
    return src;
}

/* 0x800C4FD0 */
unsigned short func_800C4FD0(unsigned char *p) {
    return H(p, 0);
}

/* 0x800C4FDC */
void func_800C4FDC(unsigned char *p, unsigned int flags) {
    unsigned int v = W(p, 0);
    if (flags & 1u) {
        v &= 0xF7FFFFFFu;
    } else {
        v |= 0x08000000u;
    }
    W(p, 0) = v;

    v = W(p, 0);
    if (flags & 2u) {
        v |= 0x02000000u;
    } else {
        v &= 0xFDFFFFFFu;
    }
    W(p, 0) = v;

    func_800C5228(p, H(p, 0));
}

/* 0x800C5058 */
int func_800C5058(void) {
    return func_800C52B8();
}

/* 0x800C5078 */
int func_800C5078(int mode) {
    if (mode == 0) {
        return func_800C5344();
    }
    return (func_800C546C() >> 29) & 1u;
}

/* 0x800C50B4 */
int func_800C50B4(int mode) {
    if (mode == 0) {
        return func_800C53D8();
    }
    return (func_800C546C() >> 24) & 1u;
}

/* 0x800C50F0 */
void func_800C50F0(void *cb) {
    func_800BA798(0, cb);
}

/* 0x800C5114 */
void func_800C5114(void *cb) {
    func_800BA798(1, cb);
}
