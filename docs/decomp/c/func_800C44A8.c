/*
 * func_800C44A8 (and unlabelled static helper at 0x800C4AA8)
 *
 * Purpose:
 *   Updates up to 24 SPU sound voices (loop over voice index 0..23 masked by
 *   W(ctx, 0x00)) in the SPU voice register buffer (*D_800D558C, 16 bytes per
 *   voice) and pitch/note table D_800D5148, according to the dirty-field
 *   bitmask W(ctx, 0x04) (0 = update all fields). Ends with a short delay loop.
 *   Also includes an unlabelled fixed-point interpolation helper at 0x800C4AA8
 *   located in the same assembly unit.
 *
 * Arguments:
 *   func_800C44A8:
 *     ctx - pointer to SPU voice attribute/command struct (mask at +0x00,
 *           flags at +0x04, volumes/modes/ADSR/pitch fields at +0x08..+0x3C)
 *   func_800C4AA8:
 *     base - base value (fixed-point input)
 *     step - step/index (upper bits step by 0x103B/4096, low 5 bits interpolate)
 *
 * Return value:
 *   func_800C44A8: None (void)
 *   func_800C4AA8: (unsigned int) interpolated fixed-point result >> 12
 *
 * Actor / global table fields read:
 *   D_800D558C (u8*) pointer to 24x 16-byte SPU voice parameter structs,
 *   D_800D5148 (u16[24]) per-voice base pitch/note table
 *
 * Global fields written:
 *   (*D_800D558C)[voice * 16 + 0x00..0x0B] SPU voice volume, pitch, ADSR fields,
 *   D_800D5148[voice]
 *
 * Scratchpad (0x1F800000) fields read/written:
 *   None
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   24 voices (0x18, PS1 SPU), 0x8000..0xE000 SPU volume sweep modes,
 *   0x103B (4155 / 4096 ~= 1.0144) quarter-semitone pitch step in 0x800C4AA8.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned short D_800D5148[];
extern unsigned char *D_800D558C;

extern void func_800C1C88(unsigned int regOffset, unsigned int val);
extern unsigned short func_800C4B34(unsigned int noteHi, unsigned int noteLo, unsigned int fineHi, unsigned int fineLo);

void func_800C44A8(unsigned char *ctx)
{
    unsigned int flags = W(ctx, 0x04);
    int updateAll = (flags < 1U);
    int voice;
    unsigned short *pitchTable = D_800D5148;
    volatile int delayVal;
    volatile int delayCnt;

    for (voice = 0; voice < 24; voice++, pitchTable++) {
        int voiceOff;

        if ((W(ctx, 0x00) & (1U << voice)) == 0) {
            continue;
        }
        voiceOff = voice << 3;

        if (updateAll || (flags & 0x10)) {
            H(D_800D558C, (voice << 4) + 0x04) = H(ctx, 0x14);
        }

        if (updateAll || (flags & 0x40)) {
            *pitchTable = H(ctx, 0x18);
        }

        if (updateAll || (flags & 0x20)) {
            unsigned short p = *pitchTable;
            unsigned short f = H(ctx, 0x16);
            unsigned short res = func_800C4B34(p >> 8, p & 0xFF, f >> 8, f & 0xFF);
            H(D_800D558C, (voiceOff << 1) + 0x04) = res;
        }

        if (updateAll || (flags & 0x01)) {
            unsigned int vol = H(ctx, 0x08) & 0x7FFF;
            unsigned int mode = 0;

            if (updateAll || (flags & 0x04)) {
                switch ((signed short)(H(ctx, 0x0C) - 1)) {
                case 0: mode = 0x8000; break;
                case 1: mode = 0x9000; break;
                case 2: mode = 0xA000; break;
                case 3: mode = 0xB000; break;
                case 4: mode = 0xC000; break;
                case 5: mode = 0xD000; break;
                case 6: mode = 0xE000; break;
                }
            }
            if (mode != 0) {
                signed short svol = S(ctx, 0x08);
                if (svol >= 0x80) {
                    vol = 0x7F;
                } else if (svol < 0) {
                    vol = 0;
                }
            }
            H(D_800D558C, (voiceOff << 1) + 0x00) = (unsigned short)(vol | mode);
        }

        if (updateAll || (flags & 0x02)) {
            unsigned int vol = H(ctx, 0x0A) & 0x7FFF;
            unsigned int mode = 0;

            if (updateAll || (flags & 0x08)) {
                switch ((signed short)(H(ctx, 0x0E) - 1)) {
                case 0: mode = 0x8000; break;
                case 1: mode = 0x9000; break;
                case 2: mode = 0xA000; break;
                case 3: mode = 0xB000; break;
                case 4: mode = 0xC000; break;
                case 5: mode = 0xD000; break;
                case 6: mode = 0xE000; break;
                }
            }
            if (mode != 0) {
                signed short svol = S(ctx, 0x0A);
                if (svol >= 0x80) {
                    vol = 0x7F;
                } else if (svol < 0) {
                    vol = 0;
                }
            }
            H(D_800D558C, (voiceOff << 1) + 0x02) = (unsigned short)(vol | mode);
        }

        if (updateAll || (flags & 0x80)) {
            func_800C1C88((unsigned int)(voiceOff | 3), W(ctx, 0x1C));
        }

        if (updateAll || (flags & 0x10000)) {
            func_800C1C88((unsigned int)(voiceOff | 7), W(ctx, 0x20));
        }

        if (updateAll || (flags & 0x20000)) {
            H(D_800D558C, (voiceOff << 1) + 0x08) = H(ctx, 0x3A);
        }

        if (updateAll || (flags & 0x40000)) {
            H(D_800D558C, (voiceOff << 1) + 0x0A) = H(ctx, 0x3C);
        }

        if (updateAll || (flags & 0x800)) {
            unsigned int ar = H(ctx, 0x30);
            unsigned int arMode = 0;
            unsigned char *entry;

            if (ar >= 0x80) {
                ar = 0x7F;
            }
            if (updateAll || (flags & 0x100)) {
                if (W(ctx, 0x24) == 5) {
                    arMode = 0x80;
                }
            }
            entry = D_800D558C + (voiceOff << 1);
            H(entry, 0x08) = (unsigned short)((H(entry, 0x08) & 0xFF) | ((ar | arMode) << 8));
        }

        if (updateAll || (flags & 0x1000)) {
            unsigned int dr = H(ctx, 0x32);
            unsigned char *entry;

            if (dr >= 0x10) {
                dr = 0x0F;
            }
            entry = D_800D558C + (voiceOff << 1);
            H(entry, 0x08) = (unsigned short)((H(entry, 0x08) & 0xFF0F) | (dr << 4));
        }

        if (updateAll || (flags & 0x2000)) {
            unsigned int sr = H(ctx, 0x34);
            unsigned int srMode = 0x100;
            unsigned char *entry;

            if (sr >= 0x80) {
                sr = 0x7F;
            }
            if (updateAll || (flags & 0x200)) {
                int mode = SW(ctx, 0x28);
                if (mode == 5) {
                    srMode = 0x200;
                } else if (mode < 6) {
                    if (mode == 1) {
                        srMode = 0;
                    }
                } else if (mode == 7) {
                    srMode = 0x300;
                }
            }
            entry = D_800D558C + (voiceOff << 1);
            H(entry, 0x0A) = (unsigned short)((H(entry, 0x0A) & 0x3F) | ((sr | srMode) << 6));
        }

        if (updateAll || (flags & 0x4000)) {
            unsigned int rr = H(ctx, 0x36);
            unsigned int rrMode = 0;
            unsigned char *entry;

            if (rr >= 0x20) {
                rr = 0x1F;
            }
            if (updateAll || (flags & 0x400)) {
                unsigned int mode = W(ctx, 0x2C);
                if (mode != 3 && mode != 7) {
                    rrMode = 0x20;
                }
            }
            entry = D_800D558C + (voiceOff << 1);
            H(entry, 0x0A) = (unsigned short)((H(entry, 0x0A) & 0xFFC0) | (rr | rrMode));
        }

        if (updateAll || (flags & 0x8000)) {
            unsigned int sl = H(ctx, 0x38);
            unsigned char *entry;

            if (sl >= 0x10) {
                sl = 0x0F;
            }
            entry = D_800D558C + (voiceOff << 1);
            H(entry, 0x08) = (unsigned short)((H(entry, 0x08) & 0xFFF0) | sl);
        }
    }

    delayVal = 1;
    delayCnt = 0;
    while (delayCnt < 2) {
        delayVal = delayVal * 13;
        delayCnt++;
    }
}

/* Unlabelled static helper at 0x800C4AA8 in the same assembly file */
unsigned int func_800C4AA8(int base, unsigned int step)
{
    int factor = 0x103B;
    unsigned int lo = (unsigned int)(base << 12);
    unsigned int hi;
    int i = 0;
    unsigned int coarse = step >> 5;
    unsigned int frac = step & 0x1F;

    hi = (unsigned int)(base * factor);
    while (i < (int)coarse) {
        lo = (unsigned int)(base * factor);
        factor = (int)(((unsigned int)(factor * 4155)) >> 12);
        hi = (unsigned int)(base * factor);
        i++;
    }

    return (lo + (((hi - lo) >> 5) * frac)) >> 12;
}
