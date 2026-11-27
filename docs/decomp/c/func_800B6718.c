/*
 * func_800B6718
 *
 * Purpose:
 *   PsyQ rcos() equivalent: fixed-point cosine lookup over a 4096-unit circle
 *   (0x1000 = 2*pi, 0x400 = pi/2, 0x800 = pi, 0xC00 = 3*pi/2). Exploits even
 *   symmetry (cos(-a) = cos(a)) and folds the angle into quarter-wave sine LUT
 *   D_800D3094 (D_800D2894 = D_800D3094 - 0x400 shorts; D_800D1894 = D_800D3094 - 0xC00 shorts).
 *
 * Arguments:
 *   angle (signed int): fixed-point angle (4096 units = full rotation)
 *
 * Return value:
 *   signed int: fixed-point cosine value (4096 = 1.0)
 *
 * Globals read:
 *   D_800D3094 (signed short[]): quarter-wave sine lookup table
 *   D_800D2894 (signed short[]): offset alias (D_800D3094 - 0x400 entries)
 *   D_800D1894 (signed short[]): offset alias (D_800D3094 - 0xC00 entries)
 */

extern signed short D_800D3094[];
extern signed short D_800D2894[];
extern signed short D_800D1894[];

signed int func_800B6718(signed int angle)
{
    unsigned int a = (angle < 0) ? (0U - (unsigned int)angle) : (unsigned int)angle;

    a &= 0xFFFU;

    if (a < 0x801U) {
        if (a < 0x401U) {
            return (signed int)D_800D3094[0x400U - a];
        }
        return -(signed int)D_800D2894[a];
    }

    if (a < 0xC01U) {
        return -(signed int)D_800D3094[0xC00U - a];
    }

    return (signed int)D_800D1894[a];
}
