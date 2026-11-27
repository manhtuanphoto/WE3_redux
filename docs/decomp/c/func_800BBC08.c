/*
 * func_800BBC08
 *
 * Purpose:
 *   Signed 16-bit argument wrapper around func_800BBC38(seq_idx, track_idx),
 *   which advances an audio/sequencer track tick counter in
 *   D_8010C2A8[seq_idx][track_idx].
 *
 * Arguments:
 *   seq_idx (signed short): sequence/bank index (sign-extended from 16 bits)
 *   track_idx (signed short): track index (sign-extended from 16 bits)
 *
 * Return value:
 *   None (void)
 */

extern void func_800BBC38(signed short seq_idx, signed short track_idx);

void func_800BBC08(signed short seq_idx, signed short track_idx)
{
    func_800BBC38((signed short)seq_idx, (signed short)track_idx);
}
