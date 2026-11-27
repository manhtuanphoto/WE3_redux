/*
 * func_800BF56C
 *
 * Purpose:
 *   PS1 SPU MIDI-note + fine-tune to SPU pitch register converter.
 *   Reads the current instrument entry from D_800F20B0 indexed by
 *   ((D_8010A3CF << 4) + D_8010A3D4) * 32, combines `fine` with instrument
 *   fine-tune (+0x5), computes semitone offset (note + 60 - base_note + carry),
 *   looks up the pitch value in the 12x16 table D_800D4F6C, and shifts by
 *   (octave - 5).
 *
 *   Note: The assembly file also contains an unlabeled trailing function at
 *   0x800BF674 (after func_800BF56C's `jr $ra`) which performs the same
 *   conversion taking `base_note` and `base_fine` as parameters. It is
 *   preserved below as a static reference helper.
 *
 * Arguments:
 *   note - MIDI note number
 *   fine - fine-tune offset (u16)
 *
 * Return value:
 *   16-bit SPU pitch register value
 *
 * Global fields read:
 *   D_8010A3CF (s8) bank/program index
 *   D_8010A3D4 (s8, D_8010A3CF + 5) tone/instrument index
 *   D_800F20B0 (u8 * pointer to 32-byte instrument/tone table)
 *     +0x4 (u8) base note
 *     +0x5 (u8) base fine-tune
 *   D_800D4F6C (u16[12][16] SPU pitch lookup table)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))

extern signed char D_8010A3CF;
extern unsigned char *D_800F20B0;
extern unsigned short D_800D4F6C[];

unsigned short func_800BF56C(int note, unsigned int fine)
{
    signed char sub_idx = *(signed char *)(&D_8010A3CF + 5);
    int inst_idx = (int)(short)(((int)D_8010A3CF << 4) + (int)sub_idx);
    const unsigned char *inst = D_800F20B0 + inst_idx * 32;
    int fine_sum = (int)(fine & 0xFFFF) + (int)B(inst, 0x05);
    int fine_step = fine_sum / 8;
    int carry;
    int fine_idx;
    int semi;
    int octave;
    int rem;
    unsigned int pitch;
    int shift;

    if (fine_step < 16) {
        carry = 0;
        fine_idx = fine_step;
    } else {
        carry = 1;
        fine_idx = fine_step - 16;
    }

    semi = (int)(short)(note + 60 - (int)B(inst, 0x04) + carry);
    octave = semi / 12;
    rem = semi - octave * 12;

    pitch = D_800D4F6C[((int)(short)rem << 4) + (int)(short)fine_idx];
    shift = (int)(short)(octave - 5);
    if (shift > 0) {
        pitch <<= shift;
    } else if (shift < 0) {
        pitch = (unsigned int)((int)pitch >> (-shift));
    }

    return (unsigned short)(pitch & 0xFFFF);
}

/*
 * UNCLEAR: Unlabeled trailing function at 0x800BF674 (immediately after
 * func_800BF56C's `jr $ra` in asm_compact, unreachable from func_800BF56C).
 */
#if 0
static unsigned short sub_800BF674(int note, short fine, unsigned char base_note, unsigned char base_fine)
{
    int fine_sum = (int)fine + (int)base_fine;
    int fine_step = fine_sum / 8;
    int carry;
    int fine_idx;
    int semi;
    int octave;
    int rem;
    unsigned int pitch;
    int shift;

    if (fine_step < 16) {
        carry = 0;
        fine_idx = fine_step;
    } else {
        carry = 1;
        fine_idx = fine_step - 16;
    }

    semi = (int)(short)(note - ((int)base_note - 60) + carry);
    octave = semi / 12;
    rem = semi - octave * 12;

    pitch = D_800D4F6C[((int)(short)rem << 4) + (int)(short)fine_idx];
    shift = (int)(short)(octave - 5);
    if (shift > 0) {
        pitch <<= shift;
    } else if (shift < 0) {
        pitch = (unsigned int)((int)pitch >> (-shift));
    }

    return (unsigned short)(pitch & 0xFFFF);
}
#endif
