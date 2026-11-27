/*
 * func_800BDCF8
 *
 * Purpose:
 *   Updates the active audio channel's left and right stereo volumes via the
 *   global command packet D_800E81B8 and func_800C3008. Sets flags to 0x06
 *   (bits 1 and 2: left/right volume), scales each signed 16-bit volume from
 *   [0..127] by 32767 / 127 (0x7FFF / 0x7F), stores them at +0x08 and +0x0A,
 *   and submits D_800E81B8 to func_800C3008.
 *
 * Arguments:
 *   left_vol  (signed short) - left channel volume (0..127 range)
 *   right_vol (signed short) - right channel volume (0..127 range)
 *
 * Return value:
 *   int - return value of func_800C3008(D_800E81B8) (0 on success, -1 on error)
 *
 * Global fields written:
 *   D_800E81B8 + 0x00 (s32) = 6 (left/right volume update mask)
 *   D_800E81B8 + 0x08 (s16) = (left_vol * 32767) / 127
 *   D_800E81B8 + 0x0A (s16) = (right_vol * 32767) / 127
 */

#define SH(p, o) (*(signed short *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char D_800E81B8[];

extern int func_800C3008(unsigned char *cmd);

int func_800BDCF8(signed short left_vol, signed short right_vol)
{
    signed int lv = ((signed int)left_vol * 32767) / 127;
    signed int rv = ((signed int)right_vol * 32767) / 127;

    SW(D_800E81B8, 0x00) = 6;
    SH(D_800E81B8, 0x08) = (signed short)lv;
    SH(D_800E81B8, 0x0A) = (signed short)rv;

    return func_800C3008(D_800E81B8);
}
