/*
 * func_800C39E8
 *
 * Purpose:
 *   Clears a PS1 SPU reverb work area in SPU RAM (analogous to PsyQ
 *   SpuClearReverbWorkArea) by transferring 0x400-byte chunks from the static
 *   buffer D_800D5178 and waiting for completion via func_800C3B88.
 *
 * Arguments:
 *   revMode: (unsigned int) reverb mode index (0..9)
 *
 * Return value:
 *   (int) 0 on success, -1 if revMode >= 10 or func_800C2F78 fails
 *
 * Reads / Writes:
 *   D_800D5114: (int) SPU transfer wait flag/mode passed to func_800C3B88 (R)
 *   D_800D5178: (u8[0x400]) zero/clear source buffer in main RAM (R)
 *   D_800D55A8: (int) temporarily cleared if == 1 and restored after transfer (R/W)
 *   D_800D55B4: (int) SPU address alignment shift (R)
 *   D_800D55C4: (int) temporarily cleared if != 0 and restored after transfer (R/W)
 *   D_800D55FC: (u32[10]) SPU reverb work-area base table (R)
 */

extern int D_800D5114;
extern unsigned char D_800D5178[];
extern int D_800D55A8;
extern int D_800D55B4;
extern int D_800D55C4;
extern unsigned int D_800D55FC[];

extern void func_800C18DC(int op, ...);
extern int func_800C2F78(unsigned int val);
extern void func_800C3B88(int arg);

int func_800C39E8(unsigned int revMode) {
    unsigned int size;
    unsigned int spuAddr;
    int savedA8;
    int restoredA8 = 0;
    int savedC4 = 0;
    int more;

    if (revMode >= 10) {
        return -1;
    }

    if (func_800C2F78(D_800D55FC[revMode]) != 0) {
        return -1;
    }

    if (revMode == 0) {
        int shift = D_800D55B4;
        size = 0x10U << shift;
        spuAddr = 0xFFF0U << shift;
    } else {
        unsigned int base = D_800D55FC[revMode];
        int shift = D_800D55B4;
        size = (0x10000U - base) << shift;
        spuAddr = base << shift;
    }

    savedA8 = D_800D55A8;
    if (savedA8 == 1) {
        D_800D55A8 = 0;
        restoredA8 = 1;
    }

    if (D_800D55C4 != 0) {
        savedC4 = D_800D55C4;
        D_800D55C4 = 0;
    }

    more = 1;
    do {
        unsigned int chunk;
        if (size <= 0x400U) {
            chunk = size;
            more = 0;
        } else {
            chunk = 0x400U;
        }

        func_800C18DC(2, spuAddr);
        func_800C18DC(1);
        func_800C18DC(3, D_800D5178, chunk);
        func_800C3B88(D_800D5114);

        size -= 0x400U;
        spuAddr += 0x400U;
    } while (more != 0);

    if (restoredA8 != 0) {
        D_800D55A8 = savedA8;
    }

    if (savedC4 != 0) {
        D_800D55C4 = savedC4;
    }

    return 0;
}
