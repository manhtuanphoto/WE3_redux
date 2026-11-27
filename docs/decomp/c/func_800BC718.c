/*
 * func_800BC718
 *
 * Purpose:
 *   Sets the SPU master left and right volumes (PsyQ SsSetMVol equivalent).
 *   Populates a stack SpuCommonAttr-like struct with mask = 3 (left & right
 *   master volume), scales each 16-bit signed argument by 129 (`(v << 7) + v`,
 *   mapping 0..127 to 0..0x3FFF), and passes the struct to func_800C3FC8.
 *
 * Arguments:
 *   left  (signed short): left master volume (typically 0..127)
 *   right (signed short): right master volume (typically 0..127)
 *
 * Return value:
 *   None (void)
 */

#define H(p, o) (*(unsigned short *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

extern void func_800C3FC8(unsigned char *attr);

void func_800BC718(signed short left, signed short right)
{
    unsigned char attr[0x28];

    W(attr, 0x00) = 3;
    H(attr, 0x04) = (unsigned short)((left << 7) + left);
    H(attr, 0x06) = (unsigned short)((right << 7) + right);

    func_800C3FC8(attr);
}
