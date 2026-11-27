/*
 * func_800BF758 - SPU/SEQ channel stereo volume computation & voice key-on trigger
 *
 * Purpose:
 *   Computes left and right SPU voice volumes for a sound channel from master/seq
 *   volume, expression, velocity, bank program L/R gains, three pan stages, and
 *   mono-mode clamp, then applies quadratic scaling (when program != 0x21), updates
 *   voice bitmasks, and triggers key-on via D_800E7600 / D_800E764C.
 *
 * Arguments:
 *   channel (u32 in $a0) - SPU/SEQ voice channel index (low byte used: 0..23).
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Globals read & written:
 *   D_800F20AC        (ptr) - struct with u8 volume at +0x18
 *   D_8010A3CA..DE          - sequence/channel state bytes (master/expr/pan/program)
 *   D_8010A3D3, D_8010A3CD  - additional pan bytes (0..127, center 64)
 *   D_8010C2A8[]      (ptr) - bank/program table; 176-byte entry (+0x58/+0x5A u16 L/R gain)
 *   D_800F0F70        (s16) - mono output flag (1 = mono max(L,R))
 *   D_8010A418/A41A   (u16) - per-channel L/R SPU volumes (16-byte stride)
 *   D_800E81E0[]      (u8)  - per-channel update flags (|= 3)
 *   D_800E78DC/E78F5        - 54-byte voice status records
 *   D_800E75F8..E764C       - 16-bit low/high voice bitmasks for SPU key-on/flags
 *
 * Magic numbers:
 *   16383 (0x3FFF) = PS1 SPU maximum voice volume
 *   127, 63        = MIDI 7-bit velocity/pan scaling divisors (center pan = 64)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define SB(p, o) (*(signed char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char *D_800F20AC;
extern unsigned char D_8010A3DE[];
extern unsigned char D_8010A3D3;
extern unsigned char D_8010A3CD;
extern unsigned char *D_8010C2A8[];
extern signed short D_800F0F70;
extern unsigned char D_8010A418[];
extern unsigned char D_8010A41A[];
extern unsigned char D_800E81E0[];
extern signed char D_800FB578;
extern unsigned int D_800D4F0C;
extern unsigned char D_800E78DC[];
extern unsigned char D_800E78F5[];
extern unsigned short D_800E75F8;
extern unsigned short D_800E75FA;
extern unsigned short D_8010CE6C;
extern unsigned short D_8010CE6E;
extern unsigned char D_8010A3DC;
extern unsigned short D_800E75FC;
extern unsigned short D_800E75FE;
extern unsigned short D_800E7600;
extern unsigned short D_800E764C;

extern void func_800C2DD8(unsigned int val);

void func_800BF758(unsigned int channel)
{
    unsigned char *seq_hdr = D_800F20AC;
    unsigned char *state_ptr = D_8010A3DE;
    int master_vol;
    unsigned int base_vol;
    signed short prog_word;
    unsigned int bank_idx;
    unsigned int prog_idx;
    unsigned char *prog_entry;
    unsigned int left_vol;
    unsigned int right_vol;
    signed char pan;
    unsigned int pan_u;
    unsigned int ch;
    unsigned short mask_lo;
    unsigned short mask_hi;
    signed char voice_count;
    int i;

    master_vol = ((int)B(seq_hdr, 0x18) * 16383 * (int)SB(state_ptr, -0x12)) / 16129;
    base_vol = (unsigned int)(master_vol * (int)SB(state_ptr, -0xC) * (int)SB(state_ptr, -0x9)) / 16129U;

    prog_word = S(state_ptr, 0);
    bank_idx = (unsigned int)(prog_word & 0xFF);
    prog_idx = (unsigned int)((prog_word & 0xFF00) >> 8);
    prog_entry = D_8010C2A8[bank_idx] + prog_idx * 176;

    if (prog_word == 0x21) {
        left_vol = base_vol;
        right_vol = base_vol;
    } else {
        left_vol = (base_vol * H(prog_entry, 0x58)) / 127U;
        right_vol = (base_vol * H(prog_entry, 0x5A)) / 127U;
    }

    /* Pan stage 1 */
    pan = SB(state_ptr, -0x8);
    pan_u = (unsigned int)(int)pan;
    if (pan_u < 64U) {
        right_vol = (right_vol * pan_u) / 63U;
    } else {
        left_vol = (left_vol * (unsigned int)(127 - (int)pan)) / 63U;
    }

    /* Pan stage 2 */
    pan = (signed char)D_8010A3D3;
    pan_u = (unsigned int)(int)pan;
    if (pan_u < 64U) {
        right_vol = (right_vol * pan_u) / 63U;
    } else {
        left_vol = (left_vol * (unsigned int)(127 - (int)pan)) / 63U;
    }

    /* Pan stage 3 */
    pan = (signed char)D_8010A3CD;
    pan_u = (unsigned int)(int)pan;
    if (pan_u < 64U) {
        right_vol = (pan_u * right_vol) / 63U;
    } else {
        left_vol = (left_vol * (unsigned int)(127 - (int)pan)) / 63U;
    }

    if (D_800F0F70 == 1) {
        if (left_vol < right_vol) {
            left_vol = right_vol;
        } else {
            right_vol = left_vol;
        }
    }

    if (S(D_8010A3DE, 0) != 0x21) {
        left_vol = (left_vol * left_vol) / 16383U;
        right_vol = (right_vol * right_vol) / 16383U;
    }

    func_800C2DD8((unsigned int)(SB(D_8010A3DE, -0x14) - SB(D_8010A3DE, -0x6)) & 0x3F);

    ch = channel & 0xFF;
    H(D_8010A41A, ch * 16) = (unsigned short)right_vol;
    H(D_8010A418, ch * 16) = (unsigned short)left_vol;
    D_800E81E0[ch] |= 3;

    if (ch < 16U) {
        mask_lo = (unsigned short)(1U << ch);
        mask_hi = 0;
    } else {
        mask_lo = 0;
        mask_hi = (unsigned short)(1U << (ch - 16U));
    }

    H(D_800E78DC, ch * 54) = 10;
    voice_count = D_800FB578;
    for (i = 0; (signed short)i < (int)voice_count; i++) {
        signed short idx = (signed short)i;
        if ((D_800D4F0C & (1U << idx)) == 0) {
            D_800E78F5[idx * 54] &= 1;
        }
    }
    D_800E78F5[ch * 54] = 2;

    D_800E75F8 |= mask_lo;
    D_800E75FA |= mask_hi;
    D_8010CE6C &= (unsigned short)(~D_800E75F8);
    D_8010CE6E &= (unsigned short)(~D_800E75FA);

    if ((D_8010A3DC & 4) != 0) {
        D_800E75FC |= mask_lo;
        D_800E75FE |= mask_hi;
    } else {
        D_800E75FC &= (unsigned short)(~mask_lo);
        D_800E75FE &= (unsigned short)(~mask_hi);
    }

    D_800E7600 = mask_lo;
    D_800E764C = mask_hi;
}
