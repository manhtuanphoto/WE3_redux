/*
 * func_800BDD88 (plus contiguous trailing helper at end of unit)
 *
 * Purpose:
 *   Selects/configures an audio channel/track index in [-9, 9] via D_800E81B8
 *   and func_800C3008. Negative inputs take their absolute value and set bit
 *   0x100 in the stored channel field (D_800E81B8+0x04 / D_800E81BC). If the
 *   absolute value >= 10, returns -1. When track == 0, calls func_800C2E28(0)
 *   before applying the config.
 *   Also includes an unlabelled trailing getter at the end of the assembly unit
 *   that returns *(signed short *)&D_800E81BC.
 *
 * Arguments:
 *   track (signed short) - track/channel index (-9..9)
 *
 * Return value:
 *   func_800BDD88: (int) absolute track index (0..9), or -1 if out of range.
 *   trailing helper: (signed short) D_800E81BC.
 *
 * Global fields read/written:
 *   D_800E81B8 (u8[8]+...) audio command packet (+0x00 = 1, +0x04 = track | flag)
 *   D_800E81BC (s16)        alias of D_800E81B8 + 0x04 low halfword
 */

#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char D_800E81B8[];
extern signed short D_800E81BC;

extern void func_800C2E28(unsigned int arg);
extern int func_800C3008(unsigned char *cmd);

int func_800BDD88(signed short track)
{
    int isNeg = 0;
    int absTrack = (int)track;
    signed short absShort;

    if (track < 0) {
        isNeg = 1;
        absTrack = -absTrack;
    }

    if ((absTrack & 0xFFFF) >= 10) {
        return -1;
    }

    SW(D_800E81B8, 0x00) = 1;
    if (isNeg != 0) {
        SW(D_800E81B8, 0x04) = (signed int)(signed short)(absTrack | 0x100);
    } else {
        SW(D_800E81B8, 0x04) = (signed int)(signed short)absTrack;
    }

    absShort = (signed short)absTrack;
    if (absShort == 0) {
        func_800C2E28(0U);
    }

    func_800C3008(D_800E81B8);
    return (int)absShort;
}

/* Unlabelled trailing helper at the end of func_800BDD88.s */
signed short func_800BDD88_get_cached_track(void)
{
    return D_800E81BC;
}
