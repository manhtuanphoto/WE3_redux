/*
 * func_800BCA78
 *
 * Purpose:
 *   Stops and resets a PS1 libsnd SEQ/SEP sequence track state struct
 *   (stride 0xB0 = 176 bytes) at D_8010C2A8[sepAccess] + seqIdx * 0xB0,
 *   calling func_800C0930 and func_800BE498, clearing playback state fields,
 *   and resetting all 16 MIDI channels to default pan (0x40) and volume (0x7F).
 *
 * Arguments:
 *   sepAccess: (signed short) SEP/SEQ access table index in D_8010C2A8
 *   seqIdx:    (signed short) sequence index within the SEP block (stride 0xB0)
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   D_8010C2A8: (u8*[]) table of SEP sequence state arrays (R)
 *   Sequence state struct (0xB0 bytes):
 *     +0x00, +0x08 (u32) reset to +0x04 (R/W)
 *     +0x14..+0x1F, +0x21 (u8), +0x88 (u32) cleared to 0 (W)
 *     +0x27..+0x36 (u8[16]) channel pan/expression reset to 0x40 (W)
 *     +0x37..+0x46 (u8[16]) channel map reset to 0..15 (W)
 *     +0x54 (u16) reset to +0x56 (R/W)
 *     +0x5C, +0x5E (u16) master L/R volume reset to 0x7F (W)
 *     +0x60..+0x7E (u16[16]) channel volumes reset to 0x7F (W)
 *     +0x90, +0x94 (u32) reset to +0x84, +0x8C (R/W)
 *     +0x98 (u32) status flags: bits 0,1,3,10 cleared; bit 2 set (R/W)
 */

#define B(p,o) (*(unsigned char*)((p)+(o)))
#define H(p,o) (*(unsigned short*)((p)+(o)))
#define W(p,o) (*(unsigned int*)((p)+(o)))

extern unsigned char *D_8010C2A8[];
extern void func_800BE498(void);
extern void func_800C0930(int packedId);

void func_800BCA78(signed short sepAccess, signed short seqIdx) {
    int seqOff = (int)seqIdx * 0xB0;
    unsigned char *seq = D_8010C2A8[sepAccess] + seqOff;
    int ch;

    W(seq, 0x98) &= ~0x001U;
    seq = D_8010C2A8[sepAccess] + seqOff;
    W(seq, 0x98) &= ~0x002U;
    seq = D_8010C2A8[sepAccess] + seqOff;
    W(seq, 0x98) &= ~0x008U;
    seq = D_8010C2A8[sepAccess] + seqOff;
    W(seq, 0x98) &= ~0x400U;
    seq = D_8010C2A8[sepAccess] + seqOff;
    W(seq, 0x98) |= 0x004U;

    func_800C0930((signed short)(((unsigned short)sepAccess) | ((unsigned short)seqIdx << 8)));
    /* UNCLEAR: Assembly clears $a3 = 0 in the delay slot of `jal func_800BE498`
     * and immediately uses $a3 as the 0..15 channel loop counter after return.
     */
    func_800BE498();

    B(seq, 0x14) = 0;
    W(seq, 0x88) = 0;
    B(seq, 0x1C) = 0;
    B(seq, 0x18) = 0;
    B(seq, 0x19) = 0;
    B(seq, 0x1E) = 0;
    B(seq, 0x1A) = 0;
    B(seq, 0x1B) = 0;
    B(seq, 0x1F) = 0;
    B(seq, 0x17) = 0;
    B(seq, 0x21) = 0;
    B(seq, 0x1C) = 0;
    B(seq, 0x1D) = 0;
    B(seq, 0x15) = 0;
    B(seq, 0x16) = 0;

    W(seq, 0x90) = W(seq, 0x84);
    W(seq, 0x94) = W(seq, 0x8C);
    H(seq, 0x54) = H(seq, 0x56);
    W(seq, 0x00) = W(seq, 0x04);
    W(seq, 0x08) = W(seq, 0x04);

    for (ch = 0; ch < 16; ch++) {
        B(seq, 0x37 + ch) = (unsigned char)ch;
        B(seq, 0x27 + ch) = 0x40;
        H(seq, 0x60 + (ch << 1)) = 0x7F;
    }

    H(seq, 0x5C) = 0x7F;
    H(seq, 0x5E) = 0x7F;
}
