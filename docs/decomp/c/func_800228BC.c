/*
 * func_800228BC
 *
 * Purpose:
 *   Classifies a player actor's priority/role category (0..5) based on player ID
 *   (+0x38D), team (+0x399), flag (+0x39A), and Scratchpad match/ball-owner state:
 *     0 - Goalkeeper (player ID 1 or 12)
 *     1 - Primary active/controlled player (Scratchpad+0x240)
 *     2 - Secondary player (Scratchpad+0x241 when Scratchpad+0x2E2 != 3)
 *     3 - Secondary player (Scratchpad+0x241 when Scratchpad+0x2E2 == 3)
 *     4 - Player matching (Scratchpad+0x290 % 22) or team slot Scratchpad+0x31E/0x31F
 *     5 - All other outfield players
 *
 * Arguments:
 *   actor - pointer to player actor struct
 *
 * Return value:
 *   Category integer 0..5
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x240 (u8) primary player ID
 *   +0x241 (u8) secondary player ID
 *   +0x290 (s32) target/focus index (taken mod 22)
 *   +0x29A (u8) match phase (checked == 1)
 *   +0x2E2 (u8) sub-mode (checked == 3)
 *   +0x2FC (u8) override/pause flag (checked == 0)
 *   +0x31E + team*2 (u8), +0x31F + team*2 (u8) designated team player IDs
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define SW(p, o) (*(int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

int func_800228BC(const unsigned char *actor)
{
    unsigned char id = B(actor, 0x38D);
    unsigned char team;
    int focus_mod;

    if (id == 1 || id == 12) {
        return 0;
    }

    if (B(actor, 0x39A) != 1 &&
        B(Scratchpad, 0x2FC) == 0 &&
        B(Scratchpad, 0x29A) == 1) {
        if (id == B(Scratchpad, 0x240)) {
            return 1;
        }
        if (id == B(Scratchpad, 0x241)) {
            return (B(Scratchpad, 0x2E2) == 3) ? 3 : 2;
        }

        focus_mod = SW(Scratchpad, 0x290) % 22;
        if ((int)(id % 22U) == focus_mod) {
            return 4;
        }
        team = B(actor, 0x399);
        if (id == B(Scratchpad, 0x31F + (int)team * 2)) {
            return 4;
        }
        return 5;
    }

    team = B(actor, 0x399);
    if (id == B(Scratchpad, 0x31E + (int)team * 2)) {
        return 4;
    }
    if (id == B(Scratchpad, 0x31F + (int)team * 2)) {
        return 4;
    }

    focus_mod = SW(Scratchpad, 0x290) % 22;
    if ((int)(id % 22U) == focus_mod) {
        return 4;
    }

    return 5;
}
