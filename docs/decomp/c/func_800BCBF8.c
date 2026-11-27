/*
 * func_800BCBF8 (plus an adjacent unlabeled function in func_800BCBF8.s)
 *
 * Purpose:
 *   Sequencer track stop wrappers calling func_800BCA78:
 *   1. func_800BCBF8(seq): stops track 0 of sequence `seq` (PsyQ SsSeqStop).
 *   2. func_800BCC20(sepAccess, seqIdx) [unlabeled at +0x28]: stops track
 *      `seqIdx` of SEP `sepAccess` (PsyQ SsSepStop).
 *
 * Arguments:
 *   sepAccess (signed short): sequence/SEP access index
 *   seqIdx    (signed short): sub-sequence index (second function only; 0 in first)
 *
 * Return value:
 *   None (void)
 */

extern void func_800BCA78(signed short sepAccess, signed short seqIdx);

void func_800BCBF8(signed short sepAccess)
{
    func_800BCA78(sepAccess, 0);
}

/* UNCLEAR: unlabeled second function inside func_800BCBF8.s (at 0x800BCC20) */
void func_800BCC20(signed short sepAccess, signed short seqIdx)
{
    func_800BCA78(sepAccess, seqIdx);
}
