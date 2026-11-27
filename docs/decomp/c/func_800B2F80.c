/*
 * func_800B2F80
 *
 * Purpose:
 *   Recursive screen-space frustum-culled quad subdivision renderer using the
 *   PS1 GTE (RTPT) to transform midpoints and center vertices, either calling
 *   func_800B3408 on the 4 leaf sub-quads at max depth or recursing into 4
 *   sub-quads via an 0x8C-byte stack workspace.
 *
 * Arguments:
 *   a0 (u32)  - passthrough return value (returned unmodified on cull or exit)
 *   a1 (u32*) - render/viewport config: +0x00 maxDepth, +0x04 width, +0x08 height
 *   a2 (s32)  - current subdivision depth
 *   a3 (u8*)  - 0x8C-byte (140-byte) per-level vertex/quad workspace:
 *               +0x00/+0x18/+0x30/+0x48/+0x60 = 5x 24-byte vertices (mid01, mid02, mid13, mid23, center)
 *               +0x78/+0x7C/+0x80/+0x84 = corner vertex pointers t0, t1, t2, t3
 *               +0x88 = saved return address across recursive calls
 *
 * Return value:
 *   u32 - returns a0 unchanged
 *
 * Reads / Writes:
 *   a1[0..2] (R): max subdivision depth, viewport width, viewport height
 *   Corner vertices t0..t3 (R):
 *     +0x00/+0x02/+0x04 (s16) 3D x/y/z
 *     +0x08/+0x09 (u8) texture u/v
 *     +0x10/+0x12 (s16) screen sx/sy
 *     +0x14 (u32) screen sz
 *   Workspace a3 (R/W): interpolated midpoint vertices (+0x00..+0x77),
 *     sub-quad corner pointers (+0x78..+0x84 in next level +0x8C)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x8C = 140-byte workspace frame stride per recursion depth
 *   0x18 = 24-byte vertex struct stride
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

/* UNCLEAR: func_800B3408 takes its 4 vertex pointers in $t0, $t1, $t2, $t3 rather than $a0..$a3. */
extern void func_800B3408(void);

unsigned int func_800B2F80(unsigned int a0, unsigned int* a1, int a2, unsigned char* a3) {
    unsigned char* t0 = (unsigned char*)W(a3, 0x78);
    unsigned char* t1 = (unsigned char*)W(a3, 0x7C);
    unsigned char* t2 = (unsigned char*)W(a3, 0x80);
    unsigned char* t3 = (unsigned char*)W(a3, 0x84);
    int gteH = 0;
    int gteOFX = 0;
    int gteOFY = 0;
    int halfH;
    unsigned int halfScreenW;
    unsigned int halfScreenH;
    int bound;
    int x0, x1, x2, x3;

    /* UNCLEAR: GTE control register reads (cfc2 $26 = H, cfc2 $24 = OFX, cfc2 $25 = OFY). */
    halfH = gteH >> 1;
    if (W(t0, 0x14) < (unsigned int)halfH &&
        W(t1, 0x14) < (unsigned int)halfH &&
        W(t2, 0x14) < (unsigned int)halfH &&
        W(t3, 0x14) < (unsigned int)halfH) {
        return a0;
    }

    halfScreenW = a1[1] >> 1;
    halfScreenH = a1[2] >> 1;
    gteOFX >>= 16;

    bound = gteOFX + (int)halfScreenW;
    if (bound < S(t0, 0x10) && bound < S(t1, 0x10) && bound < S(t2, 0x10) && bound < S(t3, 0x10)) {
        return a0;
    }
    bound = gteOFX - (int)halfScreenW;
    if (S(t0, 0x10) < bound && S(t1, 0x10) < bound && S(t2, 0x10) < bound && S(t3, 0x10) < bound) {
        return a0;
    }

    gteOFY >>= 16;
    bound = gteOFY + (int)halfScreenH;
    if (bound < S(t0, 0x12) && bound < S(t1, 0x12) && bound < S(t2, 0x12) && bound < S(t3, 0x12)) {
        return a0;
    }
    bound = gteOFY - (int)halfScreenH;
    if (S(t0, 0x12) < bound && S(t1, 0x12) < bound && S(t2, 0x12) < bound && S(t3, 0x12) < bound) {
        return a0;
    }

    /* Interpolate x, y, z for midpoints (+0x00=mid01, +0x18=mid02, +0x30=mid13, +0x48=mid23, +0x60=center) */
    x0 = S(t0, 0); x1 = S(t1, 0); x2 = S(t2, 0); x3 = S(t3, 0);
    S(a3, 0x30) = (short)((x3 + x1) >> 1);
    S(a3, 0x18) = (short)((x0 + x2) >> 1);
    S(a3, 0x00) = (short)((x0 + x1) >> 1);
    S(a3, 0x48) = (short)((x3 + x2) >> 1);
    S(a3, 0x60) = (short)(((x0 + x1) + (x3 + x2)) >> 2);

    x0 = S(t0, 2); x1 = S(t1, 2); x2 = S(t2, 2); x3 = S(t3, 2);
    S(a3, 0x32) = (short)((x3 + x1) >> 1);
    S(a3, 0x1A) = (short)((x0 + x2) >> 1);
    S(a3, 0x02) = (short)((x0 + x1) >> 1);
    S(a3, 0x4A) = (short)((x3 + x2) >> 1);
    S(a3, 0x62) = (short)(((x0 + x1) + (x3 + x2)) >> 2);

    x0 = S(t0, 4); x1 = S(t1, 4); x2 = S(t2, 4); x3 = S(t3, 4);
    S(a3, 0x34) = (short)((x3 + x1) >> 1);
    S(a3, 0x1C) = (short)((x0 + x2) >> 1);
    S(a3, 0x04) = (short)((x0 + x1) >> 1);
    S(a3, 0x4C) = (short)((x3 + x2) >> 1);
    S(a3, 0x64) = (short)(((x0 + x1) + (x3 + x2)) >> 2);

    /* UNCLEAR: PS1 GTE RTPT #1 on (a3+0x00, a3+0x18, a3+0x60):
     *   lwc2 $0..$5 <- (+0x00,+0x04,+0x18,+0x1C,+0x60,+0x64), then rtpt.
     *   Resulting SXY0..2 -> (+0x10,+0x28,+0x70) and SZ1..3 -> (+0x14,+0x2C,+0x74).
     */
    B(a3, 0x38) = (unsigned char)((B(t3, 8) + B(t1, 8)) >> 1);
    B(a3, 0x20) = (unsigned char)((B(t0, 8) + B(t2, 8)) >> 1);
    B(a3, 0x08) = (unsigned char)((B(t0, 8) + B(t1, 8)) >> 1);
    B(a3, 0x50) = (unsigned char)((B(t3, 8) + B(t2, 8)) >> 1);
    B(a3, 0x68) = (unsigned char)(((B(t0, 8) + B(t1, 8)) + (B(t3, 8) + B(t2, 8))) >> 2);

    /* UNCLEAR: PS1 GTE RTPT #2 on (a3+0x30, a3+0x48, previous V2):
     *   lwc2 $0..$3 <- (+0x30,+0x34,+0x48,+0x4C), then rtpt.
     *   Resulting SXY0..1 -> (+0x40,+0x58) and SZ1..2 -> (+0x44,+0x5C in recursive path).
     */
    B(a3, 0x39) = (unsigned char)((B(t3, 9) + B(t1, 9)) >> 1);
    B(a3, 0x21) = (unsigned char)((B(t0, 9) + B(t2, 9)) >> 1);
    B(a3, 0x09) = (unsigned char)((B(t0, 9) + B(t1, 9)) >> 1);
    B(a3, 0x51) = (unsigned char)((B(t3, 9) + B(t2, 9)) >> 1);
    B(a3, 0x69) = (unsigned char)(((B(t0, 9) + B(t1, 9)) + (B(t3, 9) + B(t2, 9))) >> 2);

    a2++;
    if (a1[0] == (unsigned int)a2) {
        /* UNCLEAR: Passes 4 vertex pointers in $t0..$t3 to func_800B3408 for each leaf sub-quad:
         *   (t0, a3+0x00, a3+0x18, a3+0x60)
         *   (t1, a3+0x30, a3+0x00, a3+0x60)
         *   (t2, a3+0x18, a3+0x48, a3+0x60)
         *   (t3, a3+0x48, a3+0x30, a3+0x60)
         */
        func_800B3408();
        func_800B3408();
        func_800B3408();
        func_800B3408();
        a2--;
    } else {
        unsigned char* next = a3 + 0x8C;

        W(next, 0x78) = (unsigned int)t0;
        W(next, 0x7C) = (unsigned int)(a3 + 0x00);
        W(next, 0x80) = (unsigned int)(a3 + 0x18);
        W(next, 0x84) = (unsigned int)(a3 + 0x60);
        func_800B2F80(a0, a1, a2, next);

        W(next, 0x78) = (unsigned int)t1;
        W(next, 0x7C) = (unsigned int)(a3 + 0x30);
        W(next, 0x80) = (unsigned int)(a3 + 0x00);
        W(next, 0x84) = (unsigned int)(a3 + 0x60);
        func_800B2F80(a0, a1, a2, next);

        W(next, 0x78) = (unsigned int)t2;
        W(next, 0x7C) = (unsigned int)(a3 + 0x18);
        W(next, 0x80) = (unsigned int)(a3 + 0x48);
        W(next, 0x84) = (unsigned int)(a3 + 0x60);
        func_800B2F80(a0, a1, a2, next);

        W(next, 0x78) = (unsigned int)t3;
        W(next, 0x7C) = (unsigned int)(a3 + 0x48);
        W(next, 0x80) = (unsigned int)(a3 + 0x30);
        W(next, 0x84) = (unsigned int)(a3 + 0x60);
        func_800B2F80(a0, a1, a2, next);

        a2--;
    }

    return a0;
}
