/*
 * func_800B700C
 *
 * Purpose:
 *   Sets the CD-ROM sync/callback pointer D_800D394C to `cb` and returns its
 *   previous value (PsyQ CdSyncCallback-style setter).
 *
 * Arguments:
 *   cb (unsigned int): new callback pointer / state word stored into D_800D394C
 *
 * Return value:
 *   unsigned int: previous value of D_800D394C
 *
 * Global fields read/written:
 *   D_800D394C (unsigned int): CD callback / state word
 */

extern unsigned int D_800D394C;

unsigned int func_800B700C(unsigned int cb)
{
    unsigned int prev = D_800D394C;
    D_800D394C = cb;
    return prev;
}
