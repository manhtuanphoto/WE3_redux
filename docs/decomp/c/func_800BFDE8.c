/*
 * func_800BFDE8 - Compute stereo SPU voice volumes and update sound voice bitmasks
 *
 * Purpose:
 *   Computes left and right SPU channel volumes for a sound voice from global
 *   master/channel volume multipliers, instrument table gains (D_8010C2A8),
 *   and three stereo pan weights (0..127), applies mono collapse and quadratic
 *   curve (vol^2 / 16383) unless program id == 0x21, stores the voice params,
 *   and updates SPU key-on / status bitmasks.
 *
 * Arguments:
 *   unused - unused register $a0 (overwritten immediately)
 *   voice_param - s16 value stored into D_8010A41C[voice*8] and D_800E78DC[ch*27]
 *
 * Return value:
 *   None (void).
 *
 * Global fields read & written:
 *   D_800F20AC    (ptr)    - pointer to struct; +0x18 (u8) volume scale
 *   D_8010A3CC..E2         - sound parameter block (offsets relative to D_8010A3E2)
 *   D_8010A3D3, D_8010A3CD - pan bytes (0..127, center ~64)
 *   D_800F0F70    (s16)    - mono mode flag (1 = collapse L/R to max)
 *   D_8010C2A8[]  (ptr[])  - bank table; entry +0x58/+0x5A (u16) L/R gain (/ 127)
 *   D_8010A418/1A/1C (s16[]) - voice L vol, R vol, param indexed by voice * 8
 *   D_800E81E0[]  (u8[])   - channel dirty flags (ORed with 7)
 *   D_800E78DC[]  (u16[])  - channel param table (stride 54 bytes = 27 halfwords)
 *   D_800E75FC/FE (u16)    - 32-bit bitmask split (low/high 16) set/cleared by D_8010A3DC & 4
 *   D_800E7600/4C (u16)    - bitmasks cleared for channel bit
 *   D_800E75F8/FA (u16)    - bitmasks set for channel bit
 *   D_8010CE6C/6E (u16)    - bitmasks cleared by ~(D_800E75F8/FA)
 *
 * Magic numbers:
 *   16129 = 127 * 127 (volume normalization divisor)
 *   16383 = 0x3FFF (quadratic volume curve divisor)
 *   127, 63 = stereo pan scale (0..63 -> scale right, 64..127 -> scale left)
 *   0x21 (33) = special voice/program id bypassing bank gain and quadratic curve
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char *D_800F20AC;
extern unsigned char D_8010A3E2[];
extern unsigned char D_8010A3D3;
extern unsigned char D_8010A3CD;
extern unsigned char D_8010A3DC;
extern unsigned char D_8010A3DE[];
extern unsigned char *D_8010C2A8[];
extern signed short D_800F0F70;
extern signed short D_8010A418[];
extern signed short D_8010A41A[];
extern signed short D_8010A41C[];
extern unsigned char D_800E81E0[];
extern signed short D_800E78DC[];
extern unsigned short D_800E75F8;
extern unsigned short D_800E75FA;
extern unsigned short D_800E75FC;
extern unsigned short D_800E75FE;
extern unsigned short D_800E7600;
extern unsigned short D_800E764C;
extern unsigned short D_8010CE6C;
extern unsigned short D_8010CE6E;

void func_800BFDE8(int unused, int voice_param)
{
    unsigned char *snd_base = D_8010A3E2;
    unsigned char *hdr = D_800F20AC;
    int vol_scale = B(hdr, 0x18);
    int v1 = (signed char)B(snd_base, -0x16);
    int prod1 = (v1 * ((vol_scale << 14) - vol_scale)) / 16129;
    int prod2 = prod1 * (signed char)B(snd_base, -0x10) * (signed char)B(snd_base, -0xD);
    unsigned int base_vol = (unsigned int)prod2 / 16129u;
    int voice_slot = S(snd_base, 0) << 3;
    unsigned short prog_raw = H(snd_base, -4);
    signed short prog_id = (signed short)prog_raw;
    unsigned int vol_l, vol_r;
    unsigned int pan;
    int ch;
    unsigned int low_mask, high_mask;
    unsigned int off_idx;
    unsigned short mask_f8, mask_fa;

    (void)unused;

    {
        unsigned int bank_idx = prog_raw & 0xFF;
        int instr_idx = (prog_id & 0xFF00) >> 8;
        unsigned char *instr = D_8010C2A8[bank_idx] + instr_idx * 176;

        if (prog_id == 0x21) {
            vol_l = base_vol;
            vol_r = base_vol;
        } else {
            vol_l = (unsigned int)(base_vol * H(instr, 0x58)) / 127u;
            vol_r = (unsigned int)(base_vol * H(instr, 0x5A)) / 127u;
        }
    }

    pan = B(snd_base, -0xC);
    if (pan < 0x40) {
        vol_r = (unsigned int)(vol_r * pan) / 63u;
    } else {
        vol_l = (unsigned int)(vol_l * (0x7F - pan)) / 63u;
    }

    pan = D_8010A3D3;
    if (pan < 0x40) {
        vol_r = (unsigned int)(vol_r * pan) / 63u;
    } else {
        vol_l = (unsigned int)(vol_l * (0x7F - pan)) / 63u;
    }

    pan = D_8010A3CD;
    if (pan < 0x40) {
        vol_r = (unsigned int)(vol_r * pan) / 63u;
    } else {
        vol_l = (unsigned int)(vol_l * (0x7F - pan)) / 63u;
    }

    if (D_800F0F70 == 1) {
        if (vol_l < vol_r) {
            vol_l = vol_r;
        } else {
            vol_r = vol_l;
        }
    }

    if (S(D_8010A3DE, 0) != 0x21) {
        vol_l = (vol_l * vol_l) / 16383u;
        vol_r = (vol_r * vol_r) / 16383u;
    }

    off_idx = (unsigned int)(voice_slot & 0xFFFF);
    D_8010A41C[off_idx] = (signed short)voice_param;
    D_8010A418[off_idx] = (signed short)vol_l;
    D_8010A41A[off_idx] = (signed short)vol_r;

    ch = S(D_8010A3DE, 4);
    D_800E81E0[ch] |= 7;
    D_800E78DC[ch * 27] = (signed short)voice_param;

    if (ch < 16) {
        low_mask = 1u << ch;
        high_mask = 0;
    } else {
        low_mask = 0;
        high_mask = 1u << (ch - 16);
    }

    if ((D_8010A3DC & 4) != 0) {
        D_800E75FC |= (unsigned short)low_mask;
        D_800E75FE |= (unsigned short)high_mask;
    } else {
        D_800E75FC &= (unsigned short)~low_mask;
        D_800E75FE &= (unsigned short)~high_mask;
    }

    D_800E7600 &= (unsigned short)~low_mask;
    D_800E764C &= (unsigned short)~high_mask;

    mask_fa = (unsigned short)(D_800E75FA | high_mask);
    D_800E75FA = mask_fa;
    mask_f8 = (unsigned short)(D_800E75F8 | low_mask);
    D_800E75F8 = mask_f8;

    D_8010CE6C &= (unsigned short)~mask_f8;
    D_8010CE6E &= (unsigned short)~mask_fa;
}
