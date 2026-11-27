/*
 * func_8005058C
 *
 * Purpose:
 *   Synchronizes up to 2 pending 22-byte (0x16) state blocks from
 *   D_800FF748+0x8E9C into D_800FF748+0x8AEA indexed by Scratchpad+0x2DD..0x2DE
 *   when Scratchpad+0x2DD != Scratchpad+0x2DE. If any byte differs, sets bit 0
 *   of D_800FF748+0x8F14 before copying.
 *
 * Arguments:
 *   None
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x2DD (u8) slot index 0 (R)
 *     +0x2DE (u8) slot index 1 (R)
 *     +0x2E2 (u8) mode determining slot count (0,2,3 -> 1; 1 -> 2; >=4 -> exit) (R)
 *   D_800FF748:
 *     +0x8AEA (u8[N][0x16]) active slot buffers (R/W)
 *     +0x8E9C (u8[2][0x16]) pending slot buffers (R)
 *     +0x8F14 (u8)          dirty/changed bitmask, bit 0 set on mismatch (R/W)
 */

#define B(p,o) (*(unsigned char*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern int func_80053728(int slot);

void func_8005058C(void) {
    unsigned char idx0 = B(Scratchpad, 0x2DD);
    unsigned char idx1 = B(Scratchpad, 0x2DE);
    unsigned char mode;
    int count;
    int i;

    if (idx0 == idx1) {
        return;
    }

    mode = B(Scratchpad, 0x2E2);
    if (mode == 1) {
        count = 2;
    } else if (mode == 0 || mode == 2 || mode == 3) {
        count = 1;
    } else {
        return;
    }

    if (count == 0) {
        return;
    }

    for (i = 0; i < count; i++) {
        if (func_80053728(i) != 0) {
            unsigned char *src = D_800FF748 + 0x8E9C + (i * 0x16);
            int j;

            for (j = 0; j < 0x16; j++) {
                unsigned char slot = B(Scratchpad, 0x2DD + i);
                unsigned char *dst = D_800FF748 + 0x8AEA + (slot * 0x16);

                if (dst[j] != src[j]) {
                    B(D_800FF748, 0x8F14) |= 0x01;
                    break;
                }
            }

            for (j = 0; j < 0x16; j++) {
                unsigned char slot = B(Scratchpad, 0x2DD + i);
                unsigned char *dst = D_800FF748 + 0x8AEA + (slot * 0x16);

                dst[j] = src[j];
            }
        }
    }
}
