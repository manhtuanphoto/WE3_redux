/*
 * func_800B5F28
 *
 * Purpose:
 *   Stores two pairs of 16-bit audio volume/level values (arg0/arg2 and
 *   arg1/arg3) into shadow globals D_800E7800 and D_800E789C. If mute/disable
 *   flag D_8010CD80 is non-zero, writes zeros to active globals D_800E81F8
 *   and D_800E8200; otherwise copies the four values to D_800E81F8 and D_800E8200.
 *
 * Arguments:
 *   arg0 (signed short) - stored at D_800E7800[0] (and D_800E81F8[0] if unmuted)
 *   arg1 (signed short) - stored at D_800E789C[0] (and D_800E8200[0] if unmuted)
 *   arg2 (signed short) - stored at D_800E7800[1] (and D_800E81F8[1] if unmuted)
 *   arg3 (signed short) - stored at D_800E789C[1] (and D_800E8200[1] if unmuted)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_8010CD80 (s16)   - mute/zero flag (non-zero zeroes active outputs)
 *   D_800E7800 (s16[2]) - shadow pair 0 (arg0, arg2)
 *   D_800E789C (s16[2]) - shadow pair 1 (arg1, arg3)
 *   D_800E81F8 (s16[2]) - active pair 0
 *   D_800E8200 (s16[2]) - active pair 1
 */

extern signed short D_8010CD80;
extern signed short D_800E7800[2];
extern signed short D_800E789C[2];
extern signed short D_800E81F8[2];
extern signed short D_800E8200[2];

void func_800B5F28(signed short arg0, signed short arg1, signed short arg2, signed short arg3)
{
    signed short mute = D_8010CD80;

    D_800E7800[0] = arg0;
    D_800E7800[1] = arg2;
    D_800E789C[0] = arg1;
    D_800E789C[1] = arg3;

    if (mute != 0) {
        D_800E81F8[0] = 0;
        D_800E81F8[1] = 0;
        D_800E8200[0] = 0;
        D_800E8200[1] = 0;
    } else {
        D_800E81F8[0] = arg0;
        D_800E81F8[1] = arg2;
        D_800E8200[0] = arg1;
        D_800E8200[1] = arg3;
    }
}
