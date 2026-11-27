/*
 * func_800AB240
 *
 * Purpose:
 *   Plays a UI/menu sound effect at a volume level looked up from the master
 *   volume scale table D_800CE738[level], passing identical left/right volumes
 *   to func_800BD468 (bank 0, prog 3, tone 2, note 0x41, fine/pan 0).
 *
 * Arguments:
 *   level (signed short): index into D_800CE738 volume table
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800CE738 (signed short[]): master volume scale table
 */

extern signed short D_800CE738[];

extern int func_800BD468(signed short arg0, signed short arg1, int arg2, int arg3, unsigned short arg4, signed short volL, signed short volR);

void func_800AB240(signed short level)
{
    signed short vol = D_800CE738[level];

    func_800BD468(0, 3, 2, 0x41, 0, vol, vol);
}
