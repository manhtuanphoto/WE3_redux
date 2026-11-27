/*
 * func_800B6684
 *
 * Purpose:
 *   Quarter-wave fixed-point sine lookup (PsyQ rsin pattern, 0x1000 = full circle):
 *     angle <  0x401:  D_800D3094[angle]
 *     angle <  0x801:  D_800D3094[0x800 - angle]
 *     angle <  0xC01: -D_800D2094[angle]   (note: D_800D2094 == D_800D3094 - 0x800)
 *     angle >= 0xC01: -D_800D3094[0x1000 - angle]
 *
 * Arguments:
 *   angle (int) - signed angle index (0x400 = 90 deg, 0x800 = 180 deg, 0x1000 = 360 deg)
 *
 * Return value:
 *   Signed 16-bit fixed-point sine value (returned as int).
 *
 * Global fields read:
 *   D_800D3094 (s16[]) - quarter-wave sine lookup table (0..0x400)
 *   D_800D2094 (s16[]) - base address 0x1000 bytes (0x800 shorts) before D_800D3094
 */

extern signed short D_800D3094[];
extern signed short D_800D2094[];

int func_800B6684(int angle)
{
    if (angle < 0x801) {
        if (angle < 0x401) {
            return (int)D_800D3094[angle];
        }
        return (int)D_800D3094[0x800 - angle];
    }

    if (angle < 0xC01) {
        return -(int)D_800D2094[angle];
    }

    return -(int)D_800D3094[0x1000 - angle];
}
