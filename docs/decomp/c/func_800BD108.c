/*
 * func_800BD108
 *
 * Purpose:
 *   Updates a sound/sequence channel ramp/envelope parameter for entry
 *   D_8010C2A8[seq_idx] + ch_idx * 176 (0xB0). Decrements the countdown timer
 *   (+0xA8), steps current value (+0x94) toward target (+0xAC) according to
 *   rate (+0x4E), recomputes scaled output (+0x54) from (+0x50 * +0x94 * 10) /
 *   (D_8010C1BC * 60) clamped to >= 1, and clears active flags 0x40 and 0x80
 *   in (+0x98) upon completion.
 *
 * Arguments:
 *   seq_idx - s16 index into D_8010C2A8 pointer table
 *   ch_idx  - s16 channel index (176-byte stride)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read / written:
 *   D_8010C2A8 (u8 *[] table of 176-byte channel arrays)
 *     +0x4E (s16) step interval (> 0: step by ±1 every +0x4E ticks; <= 0: step by |+0x4E| per tick)
 *     +0x50 (s16) base scale factor
 *     +0x54 (s16) scaled output (min 1)
 *     +0x94 (u32) current value
 *     +0x98 (u32) status flags (clears 0x40 and 0x80 when done)
 *     +0xA8 (s32) remaining tick countdown
 *     +0xAC (u32) target value
 *   D_8010C1BC (s32/u32) tempo/tick divisor base
 */

#define SH(p, o) (*(short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(int *)((p) + (o)))

extern unsigned char *D_8010C2A8[];
extern int D_8010C1BC;

void func_800BD108(short seq_idx, short ch_idx)
{
    int seq = (int)seq_idx;
    int offset = (int)ch_idx * 176;
    unsigned char *ch = D_8010C2A8[seq] + offset;
    int timer = SW(ch, 0xA8) - 1;
    short step;
    unsigned int cur, target;
    int scaled;

    SW(ch, 0xA8) = timer;
    if (timer < 0) {
        ch = D_8010C2A8[seq] + offset;
        W(ch, 0x98) &= ~0x40U;
        ch = D_8010C2A8[seq] + offset;
        W(ch, 0x98) &= ~0x80U;
        return;
    }

    step = SH(ch, 0x4E);
    if (step > 0) {
        if (timer % (int)step != 0) {
            return;
        }
        cur = W(ch, 0x94);
        target = W(ch, 0xAC);
        if (target < cur) {
            W(ch, 0x94) = cur - 1;
        } else if (cur < target) {
            W(ch, 0x94) = cur + 1;
        }
    } else {
        cur = W(ch, 0x94);
        target = W(ch, 0xAC);
        if (target < cur) {
            unsigned int next = cur + (unsigned int)(int)step;
            W(ch, 0x94) = next;
            if (next < target) {
                W(ch, 0x94) = target;
            }
        } else if (cur < target) {
            unsigned int next = cur - (unsigned int)(int)step;
            W(ch, 0x94) = next;
            if (target < next) {
                W(ch, 0x94) = target;
            }
        }
    }

    scaled = (int)((unsigned int)((int)SH(ch, 0x50) * (int)W(ch, 0x94) * 10) /
                   (unsigned int)(D_8010C1BC * 60));
    SH(ch, 0x54) = (short)scaled;
    if ((short)scaled <= 0) {
        SH(ch, 0x54) = 1;
    }

    if (SW(ch, 0xA8) == 0 || W(ch, 0x94) == W(ch, 0xAC)) {
        ch = D_8010C2A8[seq] + offset;
        W(ch, 0x98) &= ~0x40U;
        ch = D_8010C2A8[seq] + offset;
        W(ch, 0x98) &= ~0x80U;
    }
}
