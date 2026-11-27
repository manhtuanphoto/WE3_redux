/*
 * func_800AAC6C
 *
 * Purpose:
 *   Triggers SPU Key On for voice 22 (voice bitmask 0x00400000) via
 *   func_800C3B98(1, 0x00400000).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 */

extern void func_800C3B98(int keyOnOff, unsigned int voiceMask);

void func_800AAC6C(void)
{
    func_800C3B98(1, 0x00400000U);
}
