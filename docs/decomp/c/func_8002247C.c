/*
 * func_8002247C
 *
 * Purpose:
 *   Initializes match squad lineups, player condition/form arrows, and team
 *   tactical/formation nibbles in Scratchpad and global tables for both sides
 *   (0 and 1), using tournament/mode rules in Scratchpad+0x2E2 and +0x298.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   void
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x298 (u8) game state (4 triggers randomized player form/condition arrows)
 *   +0x2DD[2] (u8) per-side team ID
 *   +0x2E2 (u8) lineup/tactics source mode (0..4)
 *
 * Scratchpad fields written:
 *   +0x2FD[2] (u8) per-side formation/tactic upper 6 bits
 *   +0x301[2] (u8) per-side formation/tactic high nibble
 *   +0x303[2] (u8) per-side formation/tactic low nibble
 *
 * Global fields read/written:
 *   D_800E7620[44] (u8) squad slot mapping (2 teams * 22 players)
 *   D_800FF748+0x8E9C[44] (u8) mirrored squad slot mapping
 *   D_800FF748+0x8AEA (u8) custom/saved lineup table (stride 22 per team ID)
 *   D_800FF748+0x89BD, +0x89E8, +0xABDC (u8) team tactic/kit nibbles
 *   D_800F0420[44] (u8), D_800E78A8[44] (u8) -> copied to D_800EF788 (4-byte stride)
 *   D_800EF788 (4 bytes per player: +0, +1, +2 condition/form, +3 cleared to 0)
 *   D_800C6AF0 / D_800C6AF1 (u8, stride 2 per team ID) default team tactics
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_800E7620[];
extern unsigned char D_800F0420[];
extern unsigned char D_800E78A8[];
extern unsigned char D_800EF788[];
extern unsigned char D_800C6AF0[];
extern unsigned char D_800C6AF1[];

extern int func_8001E6B8(int n);

void func_8002247C(void)
{
    int team;
    int player;
    int teamCount;

    for (team = 0; team < 2; team++) {
        int base22 = team * 22;
        int base88 = team * 88;

        for (player = 0; player < 0x16; player++) {
            unsigned char slot;
            unsigned int idx4;

            switch (B(Scratchpad, 0x2E2)) {
            case 0:
            case 2:
            case 3:
                if (team == 0) {
                    slot = B(D_800FF748, B(Scratchpad, 0x2DD) * 22 + 0x8AEA + player);
                } else {
                    slot = (unsigned char)player;
                }
                break;

            case 1:
                slot = B(D_800FF748, B(Scratchpad, 0x2DD + team) * 22 + 0x8AEA + player);
                break;

            case 4:
            default:
                slot = (unsigned char)player;
                break;
            }

            D_800E7620[base22 + player] = slot;
            B(D_800FF748, base22 + 0x8E9C + player) = slot;

            idx4 = (unsigned int)(base88 + player * 4);
            B(D_800EF788, idx4 + 0) = D_800F0420[base22 + player];
            B(D_800EF788, idx4 + 1) = D_800E78A8[base22 + player];

            if (B(Scratchpad, 0x298) == 4) {
                unsigned char cond = (unsigned char)func_8001E6B8(5);
                switch (cond) {
                case 0:
                case 4:
                    if (func_8001E6B8(3) == 0) {
                        B(D_800EF788, idx4 + 2) = cond;
                    } else {
                        B(D_800EF788, idx4 + 2) = 2;
                    }
                    break;
                case 1:
                case 3:
                    B(D_800EF788, idx4 + 2) = cond;
                    break;
                case 2:
                    B(D_800EF788, idx4 + 2) = 2;
                    break;
                }
            }

            B(D_800EF788, idx4 + 3) = 0;
        }

        {
            unsigned char teamId = B(Scratchpad, 0x2DD + team);

            B(Scratchpad, 0x2FD + team) = D_800C6AF0[teamId * 2] >> 2;
            B(D_800FF748, 0xABDC + team) = D_800C6AF0[teamId * 2] & 3;
            B(Scratchpad, 0x301 + team) = D_800C6AF1[teamId * 2] >> 4;
            B(Scratchpad, 0x303 + team) = D_800C6AF1[teamId * 2] & 0xF;
        }
    }

    teamCount = 0;
    switch (B(Scratchpad, 0x2E2)) {
    case 0:
    case 2:
    case 3:
        teamCount = 1;
        break;
    case 1:
        teamCount = 2;
        break;
    case 4:
        return;
    }

    for (team = 0; team < teamCount; team++) {
        unsigned char teamId = B(Scratchpad, 0x2DD + team);

        B(Scratchpad, 0x2FD + team) = B(D_800FF748, 0x89BD + teamId) >> 2;
        B(D_800FF748, 0xABDC + team) = B(D_800FF748, 0x89BD + teamId) & 3;
        B(Scratchpad, 0x301 + team) = B(D_800FF748, 0x89E8 + teamId) >> 4;
        B(Scratchpad, 0x303 + team) = B(D_800FF748, 0x89E8 + teamId) & 0xF;
    }
}
