/*
 * func_800AAC24
 *
 * Purpose:
 *   Triggers SPU Key On for voice 23 (voice bitmask 0x00800000) via
 *   func_800C3B98(1, 0x00800000).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 */

extern void func_800C3B98(int keyOnOff, unsigned int voiceMask);

void func_800AAC24(void)
{
    func_800C3B98(1, 0x00800000U);
}
