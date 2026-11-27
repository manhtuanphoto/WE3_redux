/*
 * func_80079C50
 *
 * Purpose:
 *   Computes the distance from the position stored at *(Scratchpad+0x2F4) (+0x02, +0x06)
 *   to the goal line at x = +/-0x3320 (selected by *(u8 *)(obj + 0x399)), takes the
 *   integer square root >> 8, splits the resulting value into hundreds, tens, and ones
 *   decimal digits, and writes each digit << 4 (tile/UV offset) to bytes +0x03, +0x0F,
 *   and +0x1B of the HUD element returned by func_8001D004(0x46).
 *
 * Arguments:
 *   obj (unsigned char *): pointer to struct containing side/direction flag at +0x399
 *
 * Return value:
 *   None (void)
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x2F4 (unsigned char *): pointer to position struct (+0x02 s16 x, +0x06 s16 z)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define SH(p, o) (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned int func_800B1128(signed int val);
extern unsigned char *func_8001D004(unsigned int slot);

void func_80079C50(unsigned char *obj)
{
    unsigned char *pos = (unsigned char *)W(Scratchpad, 0x2F4);
    signed short x = SH(pos, 0x02);
    signed short z = SH(pos, 0x06);
    signed int dx_a;
    signed int dx_b;
    unsigned int dist;
    unsigned char *hud;
    unsigned int hundreds;
    unsigned int rem100;
    unsigned int tens;
    unsigned int ones;

    dx_a = (B(obj, 0x399) != 0) ? (x + 0x3320) : (x - 0x3320);
    dx_b = (B(obj, 0x399) != 0) ? (x + 0x3320) : (x - 0x3320);

    dist = (func_800B1128((dx_a * dx_b) + (z * z)) >> 8) & 0xFFFF;

    hud = func_8001D004(0x46);

    hundreds = (dist / 100U) & 0xFFFF;
    rem100 = (dist - hundreds * 100U) & 0xFFFF;
    tens = (rem100 / 10U) & 0xFFFF;
    ones = (dist - (dist / 10U) * 10U) & 0xFFFF;

    B(hud, 0x03) = (unsigned char)(hundreds << 4);
    B(hud, 0x0F) = (unsigned char)(tens << 4);
    B(hud, 0x1B) = (unsigned char)(ones << 4);
}
