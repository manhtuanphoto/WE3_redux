/*
 * func_80049A94
 *
 * Purpose:
 *   Clears Scratchpad+0x2FC and updates the nearest-player chaser/marker
 *   assignments for the caller's side (if mode != 2) and/or the opposing side
 *   (if mode != 0) around target (x, z), switching the first eligible candidate
 *   not equal to actor (and passing func_80052C6C == 0) into state 0x1B.
 *
 * Arguments:
 *   actor: (unsigned char*) 1000-byte actor struct
 *   mode:  (unsigned char) 0 = own side only, 2 = opponent side only, other = both
 *   flag2: (unsigned char) if nonzero, also calls func_800512B8(cand, 1)
 *   x:     (signed short) target X coordinate (raw units, 250 = 1 m)
 *   z:     (signed short) target Z coordinate (raw units, 250 = 1 m)
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   actor+0x399 (u8): attack direction / side flag (0 or 1) (R)
 *   Scratchpad+0x2FC (u8): cleared to 0 (W)
 *   Scratchpad+0x322 + 2*side (u8[2]): nearest candidate actor IDs written by func_800676C4 (R)
 *   Scratchpad+0x31E + 2*side (u8[2]): copied from Scratchpad+0x322 + 2*side (W)
 *   D_800FF748[(id % 24) * 1000]:
 *     +0x396 (u8): set to 0x1B on selected candidate (W)
 *     +0x397 (u8): set to 0 on selected candidate (W)
 *     +0x3C2 (u16): set to 0xFF on selected candidate (W)
 *
 * Magic numbers:
 *   0x3E8 (1000): actor struct stride; actor index is (id % 24).
 *   0x1B (27): chase/press actor state.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern void func_800676C4(unsigned int side, signed short x, signed short z, int arg3);
extern unsigned int func_80052C6C(unsigned char *actor);
extern void func_800512B8(unsigned char *actor, int arg1);

void func_80049A94(unsigned char *actor, unsigned int mode, unsigned int flag2, int x, int z) {
    unsigned int i;

    B(Scratchpad, 0x2FC) = 0;

    if ((mode & 0xFFu) != 2u) {
        unsigned int team = B(actor, 0x399);
        func_800676C4(team, (signed short)x, (signed short)z, 0);

        team = B(actor, 0x399);
        B(Scratchpad, 0x31E + (team << 1)) = B(Scratchpad, 0x322 + (team << 1));
        team = B(actor, 0x399);
        B(Scratchpad, 0x31F + (team << 1)) = B(Scratchpad, 0x323 + (team << 1));

        for (i = 0; i < 2; i++) {
            unsigned int idx = B(Scratchpad, 0x322 + (B(actor, 0x399) << 1) + i) % 24u;
            unsigned char *cand = D_800FF748 + idx * 1000u;
            if (cand != actor && (func_80052C6C(cand) & 0xFFu) == 0u) {
                B(cand, 0x396) = 0x1B;
                H(cand, 0x3C2) = 0xFF;
                B(cand, 0x397) = 0;
                if ((flag2 & 0xFFu) != 0u) {
                    func_800512B8(cand, 1);
                }
                break;
            }
        }
    }

    if ((mode & 0xFFu) != 0u) {
        unsigned int opp = B(actor, 0x399) ^ 1u;
        func_800676C4(opp, (signed short)x, (signed short)z, 0);

        opp = B(actor, 0x399) ^ 1u;
        B(Scratchpad, 0x31E + (opp << 1)) = B(Scratchpad, 0x322 + (opp << 1));
        opp = B(actor, 0x399) ^ 1u;
        B(Scratchpad, 0x31F + (opp << 1)) = B(Scratchpad, 0x323 + (opp << 1));

        for (i = 0; i < 2; i++) {
            unsigned int idx = B(Scratchpad, 0x322 + ((B(actor, 0x399) ^ 1u) << 1) + i) % 24u;
            unsigned char *cand = D_800FF748 + idx * 1000u;
            if ((func_80052C6C(cand) & 0xFFu) == 0u) {
                B(cand, 0x396) = 0x1B;
                H(cand, 0x3C2) = 0xFF;
                B(cand, 0x397) = 0;
                if ((flag2 & 0xFFu) != 0u) {
                    func_800512B8(cand, 1);
                }
                break;
            }
        }
    }
}
