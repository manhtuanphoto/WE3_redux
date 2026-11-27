/*
 * func_800C4B34
 *
 * Purpose:
 *   Converts a MIDI/SEQ note + fine-tune offset into a PS1 SPU pitch register
 *   value (0..0x3FFF, 0x1000 = 1.0x playback rate) relative to a base note/fine.
 *   Uses 128 fine units per semitone (1536 = 1 octave, 48 steps of 32 fine units
 *   with 2^(1/48) ~= 4155/4096 step multiplier and linear interpolation).
 *   Also includes an unlabelled trailing function in the same asm_compact unit
 *   (lines 87-167, after jr $ra) performing the inverse pitch-to-note conversion.
 *
 * Arguments (func_800C4B34):
 *   base_note (u16): base MIDI note
 *   base_fine (u16): base fine tuning (0..127)
 *   note (u16): target MIDI note
 *   fine (u16): target fine tuning (0..127)
 *
 * Return value:
 *   SPU pitch (u16, clamped to <= 0x3FFF).
 *
 * Global fields read/written:
 *   None
 */

unsigned int func_800C4B34(unsigned short base_note, unsigned short base_fine, unsigned short note, unsigned short fine)
{
    int base = ((base_note & 0xFFFF) << 7) + (base_fine & 0xFFFF);
    int target = ((note & 0xFFFF) << 7) + (fine & 0xFFFF);
    int diff = target - base;
    int abs_diff = (diff >= 0) ? diff : -diff;
    int oct = abs_diff / 1536;
    int rem = abs_diff - oct * 1536;
    unsigned int base_pitch;
    unsigned int step_factor;
    unsigned int p_lo;
    unsigned int p_hi;
    int abs_rem;
    unsigned int num_steps;
    unsigned int frac;
    unsigned int i;
    unsigned int res;

    if (diff >= 0) {
        base_pitch = 0x1000U << oct;
    } else {
        if (rem != 0) {
            oct += 1;
            rem = 0x600 - rem;
        }
        base_pitch = (unsigned int)(0x1000 >> oct);
    }

    base_pitch &= 0xFFFF;
    step_factor = 0x103B; /* ~4096 * 2^(1/48) */
    p_hi = base_pitch * step_factor;
    p_lo = base_pitch << 12;

    abs_rem = (rem >= 0) ? rem : -rem;
    num_steps = (unsigned int)abs_rem >> 5;
    frac = (unsigned int)abs_rem & 0x1F;

    if (num_steps != 0) {
        for (i = 0; (int)i < (int)num_steps; i++) {
            p_lo = p_hi;
            step_factor = (step_factor * 0x103BU) >> 12;
            p_hi = base_pitch * step_factor;
        }
    }

    res = (p_lo + ((p_hi - p_lo) >> 5) * frac) >> 12;
    if (res >= 0x4000U) {
        res = 0x3FFFU;
    }
    return res & 0xFFFF;
}

/*
 * UNCLEAR: Unlabelled trailing function in func_800C4B34.s (lines 87-167,
 * immediately following func_800C4B34's jr $ra; inverse SPU pitch-to-note helper).
 * Returns ((base_note + semitone + (msb - 12) * 12) << 8) | (base_fine + fine).
 */
unsigned int func_800C4B34_tail(unsigned short base_note, unsigned short base_fine, unsigned short pitch)
{
    unsigned int inv = (~(unsigned int)pitch) & 0xFFFF;
    int bit = 15;
    int msb = 0;
    int oct_shift;
    unsigned int base_p;
    unsigned int factor = 0x1000;
    unsigned int target_p = pitch & 0xFFFF;
    int step = 0;
    int fine_total = 0x600;
    int semitones;
    int fine_rem;

    while (bit >= 0) {
        if (((inv >> bit) & 1U) == 0) {
            msb = bit;
            break;
        }
        bit--;
    }

    oct_shift = msb - 12;
    base_p = 1U << msb;

    while (step < 48) {
        unsigned int p_lo = base_p * factor;
        unsigned int p_hi;
        unsigned int df;
        unsigned int cur_df = 0;
        unsigned int next_df;
        int k = 0;

        factor = (factor * 0x103BU) >> 12;
        p_hi = base_p * factor;
        df = (p_hi - p_lo) >> 5;
        next_df = df;

        while (k < 32) {
            if (target_p >= ((p_lo + cur_df) >> 12) && target_p < ((p_lo + next_df) >> 12)) {
                fine_total = (step << 5) + k;
                goto found;
            }
            next_df += df;
            k++;
            cur_df += df;
        }
        step++;
    }

found:
    semitones = fine_total >> 7;
    fine_rem = fine_total - (semitones << 7);

    return ((((base_note & 0xFFFF) + semitones + oct_shift * 12) << 8) |
            ((base_fine & 0xFFFF) + fine_rem));
}
