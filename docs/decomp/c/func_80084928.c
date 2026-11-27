/*
 * func_80084928
 *
 * Purpose:
 *   Evaluates match / two-leg aggregate / penalty-shootout winner, updates the
 *   away-goals/tiebreaker flag D_8010A2F0, stores the match outcome in
 *   D_800E81FC (0 = draw/team0 win depending on mode, 1 or 2 = winner), and
 *   calls func_80057EC4().
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_8010865D (u8) home/away leg order flag
 *   D_80108667 (u8) competition flags (masked with 0x60 == 0x40)
 *   D_8010866B (u8) round/stage checked == 2
 *   D_8010A2EA (u8) leg-1 team 0 score
 *   D_8010A2EB (u8) leg-1 team 1 score
 *
 * Global fields written:
 *   D_8010A2F0 (u8) tiebreak/winner flag (score1 < score0)
 *   D_800E81FC (u8) match result code (0, 1, or 2)
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x2CE (u8) penalty shootout active flag
 *   +0x2D1 (u8) team 0 regular/match score
 *   +0x2D2 (u8) team 1 regular/match score
 *   +0x2DB (u8) team 0 penalty shootout score
 *   +0x2DC (u8) team 1 penalty shootout score
 *   +0x2E1 (u8) match stage/type (checked == 2)
 *   +0x2E2 (u8) match mode (0, 2, 3 vs others)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_8010865D;
extern unsigned char D_80108667;
extern unsigned char D_8010866B;
extern unsigned char D_8010A2EA;
extern unsigned char D_8010A2EB;
extern unsigned char D_8010A2F0;
extern unsigned char D_800E81FC;

extern void func_80057EC4(void);

void func_80084928(void)
{
    unsigned int score0;
    unsigned int score1;

    if (B(Scratchpad, 0x2CE) != 0) {
        score0 = B(Scratchpad, 0x2DB);
        score1 = B(Scratchpad, 0x2DC);
        if (B(Scratchpad, 0x2E1) == 2 &&
            (D_80108667 & 0x60U) == 0x40U &&
            D_8010866B == 2) {
            D_8010A2F0 = (unsigned char)((signed int)score1 < (signed int)score0);
        }
    } else if (B(Scratchpad, 0x2E1) == 2 &&
               (D_80108667 & 0x60U) == 0x40U &&
               D_8010866B == 2) {
        unsigned int cur0 = B(Scratchpad, 0x2D1);
        unsigned int leg0 = D_8010A2EA;
        unsigned int cur1 = B(Scratchpad, 0x2D2);
        unsigned int leg1 = D_8010A2EB;

        score0 = cur0 + leg0;
        score1 = cur1 + leg1;
        if (score0 == score1) {
            if (D_8010865D != 0) {
                score0 = leg0 + (cur0 << 1);
                score1 = (leg1 << 1) + cur1;
            } else {
                score0 = (leg0 << 1) + cur0;
                score1 = (cur1 << 1) + leg1;
            }
        }
        D_8010A2F0 = (unsigned char)((signed int)score1 < (signed int)score0);
    } else {
        score0 = B(Scratchpad, 0x2D1);
        score1 = B(Scratchpad, 0x2D2);
    }

    if (score0 == score1) {
        D_800E81FC = 0;
    } else {
        unsigned char mode = B(Scratchpad, 0x2E2);
        unsigned int away_wins = ((signed int)score1 < (signed int)score0) ? 0U : 1U;
        if (mode == 0 || (unsigned int)(mode - 2) < 2U) {
            D_800E81FC = (unsigned char)away_wins;
        } else {
            D_800E81FC = (unsigned char)(away_wins << 1);
        }
    }

    func_80057EC4();
}
