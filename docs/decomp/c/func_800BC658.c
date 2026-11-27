/*
 * func_800BC658
 *
 * Purpose:
 *   Configures a single CD/Ext audio attribute field in a 40-byte SPU common
 *   attribute struct (SpuCommonAttr layout) on the stack and submits it via
 *   func_800C3FC8.
 *
 * Arguments:
 *   channel (s8 in int) - source selector (0 = CD volume/mask 0x100/0x200,
 *                         1 = Ext volume/mask 0x1000/0x2000)
 *   side    (s8 in int) - channel side selector (0 -> attr+0x18 / +0x24,
 *                         1 -> attr+0x14 / +0x20)
 *   value   (s8 in int) - sign-extended 8-bit attribute value written into the
 *                         selected struct slot
 *
 * Return value:
 *   None (void)
 *
 * Calls:
 *   func_800C3FC8(unsigned int *attr) - PsyQ SpuSetCommonAttr equivalent
 */

extern void func_800C3FC8(unsigned int *attr);

void func_800BC658(int channel, int side, int value)
{
    unsigned int attr[10];
    signed char ch = (signed char)channel;
    signed char s = (signed char)side;
    signed int val = (signed int)(signed char)value;

    /* UNCLEAR: attr[0] (mask) and the remaining attr[] fields are not pre-zeroed
     * in assembly; if ch is not 0/1 or s is not 0/1, attr[0] retains stack contents. */
    if (ch == 0) {
        if (s == 0) {
            attr[0] = 0x200U;
            attr[6] = (unsigned int)val; /* +0x18 from attr base (sp+0x28) */
        }
        if (s == 1) {
            attr[0] = 0x100U;
            attr[5] = (unsigned int)val; /* +0x14 from attr base (sp+0x24) */
        }
    }

    if (ch == 1) {
        if (s == 0) {
            attr[0] = 0x2000U;
            attr[9] = (unsigned int)val; /* +0x24 from attr base (sp+0x34) */
        }
        if (s == 1) {
            attr[0] = 0x1000U;
            attr[8] = (unsigned int)val; /* +0x20 from attr base (sp+0x30) */
        }
    }

    func_800C3FC8(attr);
}
