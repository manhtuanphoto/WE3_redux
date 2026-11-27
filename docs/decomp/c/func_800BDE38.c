/*
 * func_800BDE38 (plus 3 trailing unlabeled SPU voice helpers in func_800BDE38.s)
 *
 * Purpose:
 *   1. func_800BDE38: enables SPU/reverb state by calling func_800C2E28(1).
 *   2. func_800BDE58 (unlabeled at +0x20): updates SPU voice channel `ch` (< 24)
 *      via func_800C4D98(ch).
 *   3. func_800BDE90 (unlabeled at +0x58): sets left/right voice volumes for
 *      channel `ch` (< 24) in 16-byte-stride array D_8010A418 / D_8010A41A and
 *      sets dirty bits 0x03 in D_800E81E0[ch].
 *   4. func_800BDEF8 (unlabeled at +0xC0): queries raw left/right voice volume
 *      for channel `ch` (< 24) via func_800C4D98 and scales each by 1/129
 *      (fixed-point 0x0FE03F81) into *outLeft and *outRight.
 *
 * Global fields read / written:
 *   D_8010A418 (s16[], stride 16 bytes): voice left volume
 *   D_8010A41A (s16[], stride 16 bytes): voice right volume
 *   D_800E81E0 (u8[24]): SPU voice dirty flags
 */

extern unsigned char D_8010A418[];
extern unsigned char D_8010A41A[];
extern unsigned char D_800E81E0[];

extern signed int func_800C2E28(signed int arg0);
extern void func_800C4D98(signed int ch, ...);

signed int func_800BDE38(void)
{
    return func_800C2E28(1);
}

/* Trailing unlabeled function in func_800BDE38.s (0x800BDE58) */
signed int func_800BDE58(signed short ch)
{
    if ((unsigned short)ch >= 0x18U) {
        return -1;
    }
    func_800C4D98((signed int)ch);
    return 0;
}

/* Trailing unlabeled function in func_800BDE38.s (0x800BDE90) */
signed int func_800BDE90(signed short ch, signed short left, signed short right)
{
    signed int idx;
    signed int off;

    if ((unsigned short)ch >= 0x18U) {
        return -1;
    }

    idx = (signed int)ch;
    off = idx << 4;
    *(signed short *)(D_8010A41A + off) = right;
    *(signed short *)(D_8010A418 + off) = left;
    D_800E81E0[idx] |= 0x03;
    return 0;
}

/* Trailing unlabeled function in func_800BDE38.s (0x800BDEF8) */
signed int func_800BDEF8(signed short ch, signed short *outLeft, signed short *outRight)
{
    signed short rawLeft;
    signed short rawRight;

    if ((unsigned short)ch >= 0x18U) {
        return -1;
    }

    func_800C4D98((signed int)ch, &rawLeft, &rawRight);
    /* Signed division by 129 using fixed-point multiplier 0x0FE03F81 */
    *outLeft = (signed short)((signed int)rawLeft / 129);
    *outRight = (signed short)((signed int)rawRight / 129);
    return 0;
}
