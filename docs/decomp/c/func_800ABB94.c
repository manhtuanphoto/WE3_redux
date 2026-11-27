/*
 * func_800ABB94
 *
 * Purpose:
 *   Blocking CD-XA / audio fade-out loop. Repeatedly decrements fade volume
 *   D_800E6AD6 by 15 (0xF) per frame until <= 0. On each active step, mirrors
 *   D_800E6ADC = D_800E6AD6, scales by master volume D_800E6B6C ((prod >> 7) with
 *   negative rounding +0x7F and s16 truncation), applies stereo volume via
 *   func_800BCC58(0, scaled, scaled), and waits a frame via func_800BA518(0).
 *   Once D_800E6AD6 <= 0, clears D_800E6ADC, D_800E6AD6, D_800E6B4C, and
 *   D_800E6B76 to 0 and stops playback via func_800A95E8().
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Globals read / written:
 *   D_800E6AD6 (s16, R/W): left fade volume
 *   D_800E6ADC (s16, W): right fade volume
 *   D_800E6B6C (s16, R): master volume scale
 *   D_800E6B4C (s16, W): CD playback state (cleared to 0)
 *   D_800E6B76 (s16, W): playback flag/counter (cleared to 0)
 */

extern signed short D_800E6AD6;
extern signed short D_800E6ADC;
extern signed short D_800E6B6C;
extern signed short D_800E6B4C;
extern signed short D_800E6B76;

extern void func_800A95E8(void);
extern void func_800BCC58(signed int ch, signed int vol_l, signed int vol_r);
extern void func_800BA518(signed int mode);

void func_800ABB94(void)
{
    for (;;) {
        signed short next_vol = (signed short)(D_800E6AD6 - 0xF);
        signed int scaled;

        D_800E6AD6 = next_vol;
        if (next_vol <= 0) {
            D_800E6ADC = 0;
            D_800E6AD6 = 0;
            D_800E6B4C = 0;
            func_800A95E8();
            D_800E6B76 = 0;
            break;
        }

        D_800E6ADC = next_vol;
        scaled = (signed int)next_vol * (signed int)D_800E6B6C;
        if (scaled < 0) {
            scaled += 0x7F;
        }
        scaled = (signed short)(scaled >> 7);

        func_800BCC58(0, scaled, scaled);
        func_800BA518(0);
    }
}
