/*
 * func_80019B64 - Commentary/VRAM upload step state machine (Scratchpad +0x29F)
 *
 * Purpose:
 *   Drives a 3-state transition sequence keyed by Scratchpad byte 0x1F80029F:
 *     - State 0: sets commentary state D_800F106E = 1, advances 0x1F80029F to 1,
 *       returns 0.
 *     - State 1: polls func_80017684(clip_id & 0xFF); once it returns non-zero,
 *       runs VRAM command list func_80019564(D_80110000, 0), resets
 *       D_800F106E = 1, increments 0x1F80029F, and returns 0.
 *     - State 2: calls func_800198E8(5) and returns its low byte.
 *
 * Arguments:
 *   clip_id (unsigned int): commentary clip ID passed (masked to 8 bits) to func_80017684
 *
 * Return value:
 *   unsigned int: 0 in states 0 and 1; (func_800198E8(5) & 0xFF) in state 2
 *
 * Scratchpad offsets read/written:
 *   0x1F80029F (u8, R/W): state machine phase (0, 1, 2)
 *
 * Globals read & written:
 *   D_800F106E (u8, W): commentary playback state set to 1 in states 0 and 1
 *   D_80110000 (u8 *, R): VRAM upload command list pointer passed to func_80019564
 */

#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800F106E;
extern unsigned char *D_80110000;

extern unsigned int func_80017684(unsigned int clip_id);
extern void func_80019564(unsigned char *cmds, unsigned int flag);
extern unsigned int func_800198E8(int arg0);

unsigned int func_80019B64(unsigned int clip_id)
{
    unsigned char state = Scratchpad[0x29F];

    if (state == 0) {
        D_800F106E = 1;
        Scratchpad[0x29F] = (unsigned char)(state + 1u);
        return 0;
    }

    if (state == 1) {
        if (func_80017684(clip_id & 0xFFu) != 0) {
            func_80019564(D_80110000, 0);
            D_800F106E = 1;
            Scratchpad[0x29F] = (unsigned char)(Scratchpad[0x29F] + 1u);
        }
        return 0;
    }

    if (state == 2) {
        return func_800198E8(5) & 0xFFu;
    }

    /* UNCLEAR: for state >= 3, assembly falls through returning leftover $v0 (2) */
    return 2;
}
