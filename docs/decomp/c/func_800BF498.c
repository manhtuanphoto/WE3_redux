/*
 * func_800BF498 - Compute SPU ADPCM pitch value from MIDI note and fine-tuning
 *
 * Purpose:
 *   Reads current note (D_8010A3CA+0x00), base center note (D_8010A3CA+0x0E),
 *   and fine-tuning (D_8010A3CA+0x0F) from the active tone/voice state at
 *   D_8010A3CA, computes `diff = note - (center_note - 60)`, splits `diff`
 *   into octave (`diff / 12`) and semitone (`diff % 12`), indexes the 12x16
 *   SPU pitch lookup table D_800D4F6C by `[semitone * 16 + fine_step]`, and
 *   scales by `2^(octave - 5)` to produce the 16-bit SPU pitch register value.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   unsigned short: 16-bit SPU pitch value
 *
 * Globals read:
 *   D_8010A3CA (u8[]): active tone parameters:
 *     +0x00 (s8): target note
 *     +0x0E (s8): base/center note (offset by -60 = -0x3C)
 *     +0x0F (s8): fine-tune (divided by 8, clamped to < 16)
 *   D_800D4F6C (u16[]): 12-semitone x 16-fine-step SPU pitch lookup table
 */

#define SB(p, o) (*(signed char *)((p) + (o)))

extern unsigned char D_8010A3CA[];
extern unsigned short D_800D4F6C[];

unsigned short func_800BF498(void)
{
    signed int note = (signed int)SB(D_8010A3CA, 0x00);
    signed int fine = (signed int)SB(D_8010A3CA, 0x0F);
    signed int center = (signed int)SB(D_8010A3CA, 0x0E);
    signed int diff;
    signed int octave;
    signed int semitone;
    unsigned int fine_step;
    signed int pitch;
    signed int oct_shift;

    diff = (signed short)(note - (center - 0x3C));

    if (fine < 0) {
        fine += 7;
    }
    /* Assembly uses logical shift `srl $a1, $a1, 3` and masks `& 0xFFFF` */
    fine_step = ((unsigned int)fine >> 3) & 0xFFFFu;
    if (fine_step >= 0x10u) {
        fine_step = 0xFu;
    }

    octave = diff / 12;
    semitone = diff - octave * 12;

    pitch = (signed int)D_800D4F6C[(semitone * 16) + (signed int)fine_step];
    oct_shift = (signed short)(octave - 5);

    if (oct_shift > 0) {
        pitch <<= oct_shift;
    } else if (oct_shift < 0) {
        pitch >>= -oct_shift;
    }

    return (unsigned short)(pitch & 0xFFFF);
}
