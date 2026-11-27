/*
 * func_800732B0
 *
 * Purpose:
 *   Handles automatic tactical slot re-assignment and bench substitution after
 *   a player dismissal/injury (identified by Scratchpad+0x2AD and team bit
 *   Scratchpad+0x2DF & 1). Maps the current formation (Scratchpad+0x2FE, 0..17)
 *   to one of 8 fallback priority groups via jtbl_80012D7C, finds an available
 *   player in D_800CA484 (up to 3 fallback slots) whose availability byte
 *   (+0x88BB) < 2, swaps them into the vacated slot in D_800FF748+0x8EB2,
 *   updates each active player's role (+0x391) from D_800C8118, and if the two
 *   swapped slots differ in D_800E78BE, brings on an eligible bench substitute
 *   (slots 11..20, non-GK bit 0x10 clear in D_800EF7E3) and flags Scratchpad+0x335.
 *
 * Arguments:
 *   void
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x2AD (u8)  affected actor ID (1..11 vs 12..22) (R)
 *     +0x2DE (u8)  team/mode index (R)
 *     +0x2DF (u8)  side bitfield (bit 0 = team 0/1) (R)
 *     +0x2E0 (u8)  attacking/own side flag (XOR 1 used) (R)
 *     +0x2E1 (u8)  mode (R)
 *     +0x2E2 (u8)  lock/skip flag (1 = early return) (R)
 *     +0x2FE (u8)  formation index (0..17) (R)
 *     +0x335 (u8)  substitution pending flag (W = 1)
 *   Globals:
 *     D_800FF748 + 0x8EB2 (u8[] slot-to-player-id map) (R/W)
 *     D_800FF748 + 0x88BB (u8[] player availability/card count) (R)
 *     D_800FF748 + 0x93A9 (u8[] eligibility table) (R)
 *     D_800FF748 + 0x3E8 * idx + 0x391/+0x392 (player role/team slot) (R/W)
 *     D_800CA484 (fallback slot priority table) (R)
 *     D_800C8118 (formation role table, stride 11) (R)
 *     D_800E78BE (u8[] position group table) (R)
 *     D_800EF7E3 (u8[] player flags, bit 0x10 = GK) (R)
 *     D_800E7663 (D_800E7660+3, outgoing player) (W)
 *     D_8010A5DB (D_8010A5D8+3, incoming substitute) (W)
 *
 * Magic numbers:
 *   0x12 = 18 supported formations (0..17)
 *   0x0C = 12 (actor ID offset for team 1)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_800CA484[];
extern unsigned char D_800C8118[];
extern unsigned char D_800E78BE[];
extern unsigned char D_800EF7E3[];
extern unsigned char D_800E7660[];
extern unsigned char D_8010A5D8[];

void func_800732B0(void) {
    unsigned int sideBit;
    unsigned char formation;
    int group;
    int rawSlot;
    unsigned int slot;
    unsigned char* candTbl;
    int attempt;
    unsigned char targetSlot = 0;
    unsigned char* targetMapPtr = (unsigned char*)0;
    unsigned char targetPlayer = 0;
    unsigned char curPlayer;
    int oppSide;
    int teamStart;
    unsigned char* actorField;
    int i, j;

    if (B(Scratchpad, 0x2E2) == 1) {
        return;
    }

    sideBit = B(Scratchpad, 0x2DF) & 1U;
    if (sideBit == 0 && B(Scratchpad, 0x2AD) < 0xCU) {
        return;
    }
    if (sideBit == 1 && B(Scratchpad, 0x2AD) >= 0xCU) {
        return;
    }

    formation = B(Scratchpad, 0x2FE);
    if (formation >= 0x12U) {
        return;
    }

    switch (formation) {
    case 0:
    case 3:
    case 11:
    case 13:
        group = 0;
        break;
    case 1:
        group = 1;
        break;
    case 2:
        group = 2;
        break;
    case 12:
    case 16:
    case 17:
        group = 3;
        break;
    case 4:
    case 6:
    case 7:
    case 14:
    case 15:
        group = 4;
        break;
    case 5:
        group = 5;
        break;
    case 8:
        group = 6;
        break;
    case 9:
    case 10:
        group = 7;
        break;
    default:
        return;
    }

    if ((B(Scratchpad, 0x2DF) & 1U) == 0) {
        rawSlot = B(Scratchpad, 0x2AD) - 0xC;
    } else {
        rawSlot = B(Scratchpad, 0x2AD) - 1;
    }
    slot = (unsigned int)(rawSlot & 0xFF);
    if (slot == 0) {
        return;
    }

    candTbl = D_800CA484 + group * 30;
    for (attempt = 0; attempt < 3; attempt++) {
        targetSlot = B(candTbl + slot, -1);
        if (targetSlot == 0) {
            return;
        }
        targetMapPtr = D_800FF748 + targetSlot + 0x8EB2;
        targetPlayer = B(targetMapPtr, 0);
        if (B(D_800FF748 + targetPlayer, 0x88BB) < 2U) {
            break;
        }
        if (attempt == 2) {
            return;
        }
        candTbl += 10;
    }

    curPlayer = B(D_800FF748 + slot, 0x8EB2);
    B(targetMapPtr, 0) = curPlayer;
    B(D_800FF748 + slot, 0x8EB2) = targetPlayer;

    oppSide = (int)(B(Scratchpad, 0x2E0) ^ 1U);
    teamStart = (oppSide * 11) % 24;
    actorField = D_800FF748 + teamStart * 0x3E8 + 0x779; /* actor[teamStart + 1] + 0x391 */

    for (i = 0; i < 11; i++) {
        unsigned char actorSlot = B(actorField, 1); /* +0x392 */
        for (j = 0; j < 11; j++) {
            if (actorSlot == B(D_800FF748 + j, 0x8EB2)) {
                B(actorField, 0) = B(D_800C8118, B(Scratchpad, 0x2FE) * 11 + j);
            }
        }
        actorField += 0x3E8;
    }

    if (B(D_800E78BE, slot) == B(D_800E78BE, targetSlot)) {
        return;
    }

    for (i = 11; i < 21; i++) {
        if (B(D_800E78BE, slot) == B(D_800E78BE, i)) {
            unsigned char benchPlayer = B(D_800FF748 + i, 0x8EB2);
            if ((unsigned int)(B(Scratchpad, 0x2E1) - 1) < 2U) {
                unsigned char modeIdx = B(Scratchpad, 0x2DE);
                if (B(D_800FF748 + benchPlayer * 6 + modeIdx * 132, 0x93A9) != 0) {
                    return;
                }
            }
            if (B(D_800FF748 + i, 0x88BB) >= 2U) {
                return;
            }
            if ((B(D_800EF7E3, benchPlayer * 4) & 0x10U) != 0) {
                return;
            }
            B(D_800E7660, 3) = B(D_800FF748 + slot, 0x8EB2);
            B(Scratchpad, 0x335) = 1;
            B(D_8010A5D8, 3) = benchPlayer;
            B(D_800FF748 + i, 0x8EB2) = B(D_800FF748 + slot, 0x8EB2);
            B(D_800FF748 + slot, 0x8EB2) = benchPlayer;
            return;
        }
    }
}
