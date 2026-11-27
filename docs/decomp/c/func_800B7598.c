/*
 * func_800B7598
 *
 * Purpose:
 *   Converts a PS1 CD-ROM BCD MSF position (CdlLOC: minute, second, sector)
 *   into a linear LBA sector index (equivalent to PsyQ CdPosToInt). Decodes
 *   BCD digits for minutes (loc[0]), seconds (loc[1]), and sectors (loc[2]),
 *   and computes ((min * 60 + sec) * 75 + sector) - 150 (subtracting the
 *   2-second = 150-sector CD pregap).
 *
 * Arguments:
 *   loc (const unsigned char *) - pointer to CdlLOC (loc[0]=BCD min, loc[1]=BCD sec, loc[2]=BCD sector)
 *
 * Return value:
 *   int - linear CD sector number (LBA)
 */

int func_800B7598(const unsigned char *loc)
{
    unsigned int m = loc[0];
    unsigned int s = loc[1];
    unsigned int f = loc[2];

    unsigned int min = (m >> 4) * 10u + (m & 0xFu);
    unsigned int sec = (s >> 4) * 10u + (s & 0xFu);
    unsigned int frm = (f >> 4) * 10u + (f & 0xFu);

    unsigned int total_sec = min * 60u + sec;
    return (int)(total_sec * 75u + frm) - 150;
}
