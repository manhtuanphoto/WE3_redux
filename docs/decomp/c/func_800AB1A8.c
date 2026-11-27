/*
 * func_800AB1A8
 *
 * Purpose:
 *   Looks up a 16-bit audio/volume parameter from table D_800CE738[idx],
 *   caches it in D_800E6B6C, scales it against signed 16-bit weights
 *   D_800E6AD6 and D_800E6ADC (divided by 128 with signed rounding toward
 *   zero, truncated to s16), passes the scaled pair to func_800BCC58(0, ...),
 *   and calls func_800AB240(idx).
 *
 * Arguments:
 *   idx (signed short) - table index into D_800CE738
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800CE738 (u16[]) - lookup table indexed by idx
 *   D_800E6AD6 (s16)   - first scale factor
 *   D_800E6ADC (s16)   - second scale factor
 *   D_800E6B6C (u16)   - cached D_800CE738[idx] value
 */

extern unsigned short D_800CE738[];
extern signed short D_800E6AD6;
extern signed short D_800E6ADC;
extern unsigned short D_800E6B6C;

extern void func_800BCC58(int arg0, int arg1, int arg2);
extern void func_800AB240(int idx);

void func_800AB1A8(signed short idx)
{
    int index = (int)idx;
    unsigned short raw = D_800CE738[index];
    signed int val = (signed short)raw;
    signed int prod1;
    signed int prod2;
    signed short scaled1;
    signed short scaled2;

    D_800E6B6C = raw;

    prod1 = (signed int)D_800E6AD6 * val;
    if (prod1 < 0) {
        prod1 += 0x7F;
    }
    scaled1 = (signed short)(prod1 >> 7);

    prod2 = (signed int)D_800E6ADC * val;
    if (prod2 < 0) {
        prod2 += 0x7F;
    }
    scaled2 = (signed short)(prod2 >> 7);

    func_800BCC58(0, (int)scaled1, (int)scaled2);
    func_800AB240(index);
}
