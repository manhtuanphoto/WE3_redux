/*
 * func_800BBC38
 *
 * Purpose:
 *   Advances an audio/sequencer track tick counter in D_8010C2A8[seq_idx][track_idx]
 *   (176-byte / 0xB0 stride per track). Compares accumulator (+0x90 s32) against period
 *   (+0x54 s16): if acc > period, steps countdown (+0x52 s16) or subtracts period from
 *   acc; otherwise invokes func_800BBF78(seq_idx, track_idx) repeatedly while
 *   acc + delta < period (with delta read from +0x90) and stores the remainder at +0x90.
 *
 * Arguments:
 *   seq_idx (signed short): sequence/bank index into D_8010C2A8
 *   track_idx (signed short): track index (0xB0-byte stride)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_8010C2A8 (unsigned char *[]): array of pointers to track tables (0xB0-byte stride)
 */

#define SH(p, o) (*(signed short *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char *D_8010C2A8[];

extern void func_800BBF78(signed short seq_idx, signed short track_idx);

void func_800BBC38(signed short seq_idx, signed short track_idx)
{
    unsigned char *track = D_8010C2A8[seq_idx] + ((signed int)track_idx * 0xB0);
    signed short period = SH(track, 0x54);
    signed int acc = SW(track, 0x90);
    signed int diff = acc - (signed int)period;

    if (diff > 0) {
        signed short cnt = SH(track, 0x52);
        if (cnt > 0) {
            SH(track, 0x52) = (signed short)(cnt - 1);
        } else if (cnt == 0) {
            SH(track, 0x52) = period;
            SW(track, 0x90) = acc - 1;
        } else {
            SW(track, 0x90) = diff;
        }
        return;
    }

    if ((signed int)period < acc) {
        /* Only reachable on 32-bit signed overflow in (acc - period) */
        return;
    }

    {
        signed int total = acc;
        signed int delta;
        signed short cur_period;

        for (;;) {
            func_800BBF78(seq_idx, track_idx);
            delta = SW(track, 0x90);
            if (delta == 0) {
                continue;
            }
            cur_period = SH(track, 0x54);
            total += delta;
            if (total >= (signed int)cur_period) {
                SW(track, 0x90) = total - (signed int)cur_period;
                break;
            }
        }
    }
}
