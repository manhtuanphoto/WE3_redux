/*
 * func_80059420
 *
 * Purpose:
 *   Triggers match commentary / crowd SFX via func_800AB620 based on match
 *   mode, score differential, ball Z position, and event flag (goal/shot/etc.).
 *
 * Arguments:
 *   actor (u8*): actor struct pointer (uses +0x398 team index)
 *   flag  (u32): low byte non-zero for goal/score-diff commentary path;
 *                0 for shot/save/general event commentary path
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800C7D88 (u16[5]) SFX ID table
 *   jtbl_80011F28 / jtbl_80011F40 jump tables in .rodata
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x2AA (u8)  set-piece / match event state
 *   +0x2B0 (u32) sub-event state (checked == 9)
 *   +0x2DB (u8)  team 0 score
 *   +0x2DC (u8)  team 1 score
 *   +0x2E2 (u8)  match mode (1 or 4 uses actor team perspective)
 *   +0x2F4 (u32) ball pointer (reads +0x06 Z coord)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned short D_800C7D88[];

extern int  func_8001E6B8(int max);
extern void func_800AB620(unsigned int sfx);
extern unsigned int func_80083D94(unsigned char s0, unsigned char s1);

void func_80059420(unsigned char *actor, unsigned int flag)
{
    unsigned char mode = B(Scratchpad, 0x2E2);
    unsigned int team;
    signed int score_diff;
    unsigned int sfx;

    if (mode == 1 || mode == 4) {
        team = B(actor, 0x398);
        score_diff = (signed int)B(Scratchpad, 0x2DB + team)
                   - (signed int)B(Scratchpad, 0x2DB + (team ^ 1U));
    } else {
        team = 0;
        score_diff = (signed int)B(Scratchpad, 0x2DB)
                   - (signed int)B(Scratchpad, 0x2DC);
    }

    if ((flag & 0xFFU) != 0) {
        func_800AB620((unsigned int)(func_8001E6B8(9) + 0xEC2));
        if (func_8001E6B8(2) != 0) {
            return;
        }
        if ((func_80083D94(B(Scratchpad, 0x2DB), B(Scratchpad, 0x2DC)) & 0xFFU) != 0) {
            return;
        }

        if (team == B(actor, 0x398)) {
            unsigned int idx = (unsigned int)(score_diff + 1);
            if (idx >= 5U) {
                return;
            }
            /* UNCLEAR: exact jtbl_80011F28 case permutation for idx 0..4; sequential branch targets shown */
            switch (idx) {
            case 0:
                sfx = 0xED4;
                break;
            case 1:
                sfx = 0xED9;
                break;
            case 2:
                sfx = 0xED3;
                break;
            case 3:
                sfx = (unsigned int)(func_8001E6B8(2) + 0xED6);
                break;
            case 4:
                func_800AB620(0xED7);
                sfx = 0xED8;
                break;
            default:
                return;
            }
        } else {
            unsigned int idx = (unsigned int)(score_diff + 2);
            if (idx >= 6U) {
                return;
            }
            /* UNCLEAR: exact jtbl_80011F40 case mapping for idx 0..5 (only 4 unique targets 0xEDC, 0xEDB, 0xEDA, 0xEDE in .s) */
            switch (idx) {
            case 0:
                sfx = 0xEDC;
                break;
            case 1:
                sfx = 0xEDB;
                break;
            case 2:
                sfx = 0xEDA;
                break;
            case 3:
                sfx = 0xEDE;
                break;
            default:
                return;
            }
        }
        func_800AB620(sfx);
        return;
    }

    {
        unsigned int state = B(Scratchpad, 0x2AA);
        if (state == 1 || state == 0xC) {
            sfx = D_800C7D88[func_8001E6B8(5)];
        } else if (func_8001E6B8(2) != 0) {
            unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
            signed short bz = S(ball, 6);
            if (bz < 0) {
                bz = (signed short)-bz;
            }
            if (bz < 0x3B2 && W(Scratchpad, 0x2B0) == 9) {
                sfx = 0xECB;
            } else {
                sfx = 0xECC;
            }
        } else {
            sfx = (unsigned int)(func_8001E6B8(2) + 0xECD);
        }
        func_800AB620(sfx);
    }
}
