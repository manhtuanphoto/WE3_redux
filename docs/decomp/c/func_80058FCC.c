/*
 * func_80058FCC
 *
 * Purpose:
 *   Triggers goal-score and lead/equalizer/blowout commentary audio clips via
 *   func_800AB620 and func_8005A1F0 based on the scoring side's current goal
 *   tally (Scratchpad+0x2D1[side]), opponent tally (Scratchpad+0x2D1[side^1]),
 *   goal increment `delta`, and player/home-side mode (Scratchpad+0x2E2).
 *
 * Arguments:
 *   actor (unsigned char *) - scoring player actor pointer (+0x398 = side 0/1)
 *   delta (unsigned char)   - goal increment added to scoring team's tally
 *
 * Return value:
 *   None (void)
 *
 * Scratchpad (0x1F800000) / Global tables read:
 *   Scratchpad +0x2D1[2] (u8) - team 0 / team 1 goal counts
 *   Scratchpad +0x2DD[2] (u8) - team 0 / team 1 commentary team IDs
 *   Scratchpad +0x2E2    (u8) - player/camera mode (1 or 4 -> refSide = side, else 0)
 *   D_800C7D72           (u16[]) - commentary clip IDs for opponent lead < 4
 *   D_800C7D7C           (u16[]) - commentary clip IDs for high-score (>= 8) games
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define SCRATCHPAD ((unsigned char *)0x1F800000)

extern unsigned short D_800C7D72[];
extern unsigned short D_800C7D7C[];

extern void func_800AB620(int sfx_id);
extern void func_8005A1F0(unsigned int team_id, int clip_base);
extern int func_8001E6B8(int range);

void func_80058FCC(unsigned char *actor, unsigned char delta)
{
    unsigned char *scratch = SCRATCHPAD;
    unsigned int side = B(actor, 0x398);
    unsigned int opp = side ^ 1U;
    int diff = (int)B(scratch, 0x2D1 + side) - (int)B(scratch, 0x2D1 + opp) + (int)(delta & 0xFFU);
    unsigned char mode;
    unsigned int ref_side;
    unsigned int new_goals;
    unsigned int opp_goals;

    if (diff < 0) {
        return;
    }

    mode = B(scratch, 0x2E2);
    if (mode == 1 || mode == 4) {
        ref_side = side;
    } else {
        ref_side = 0;
    }

    side = B(actor, 0x398);
    new_goals = (unsigned int)B(scratch, 0x2D1 + side) + (unsigned int)(delta & 0xFFU);
    opp = side ^ 1U;
    opp_goals = B(scratch, 0x2D1 + opp);

    if ((int)new_goals < 8 && opp_goals < 8U) {
        int base_off = 0;
        int i;

        for (i = 0; i < (int)opp_goals; i++) {
            base_off = (base_off + 8) - i;
        }
        func_800AB620(base_off + diff + 0xE4B);

        if (diff == 0) {
            return;
        }

        side = B(actor, 0x398);
        if (ref_side != side) {
            func_8005A1F0(B(scratch, 0x2DD + (side ^ 1U)), 0xFD6);
            if (diff >= 4) {
                func_800AB620(func_8001E6B8(2) + 0xE93);
            } else if (func_8001E6B8(2) != 0) {
                func_800AB620(D_800C7D72[diff]);
            } else {
                func_800AB620(0xE90);
            }
        } else if (diff >= 4) {
            func_8005A1F0(B(scratch, 0x2DD + ref_side), 0x1006);
            func_800AB620(func_8001E6B8(3) + 0xE85);
        } else if (func_8001E6B8(2) != 0) {
            if (diff == 0) {
                func_800AB620(0xE8F);
            } else if (diff == 3) {
                func_800AB620(0xE8C);
                func_8005A1F0(B(scratch, 0x2DD + B(actor, 0x398)), 0xFD6);
                func_800AB620(0xE8D);
            } else {
                func_8005A1F0(B(scratch, 0x2DD + B(actor, 0x398)), 0xFD6);
                func_800AB620(diff + 0xE89);
            }
        } else {
            func_800AB620(0xE88);
            func_8005A1F0(B(scratch, 0x2DD + B(actor, 0x398)), 0xFD6);
            func_800AB620(0xE89);
        }
    } else {
        unsigned int cur_goals = B(scratch, 0x2D1 + B(actor, 0x398));
        unsigned int sum = cur_goals + (unsigned int)(delta & 0xFFU);
        if ((int)sum < 0x10) {
            func_800AB620((int)sum + 0xE6F);
            func_800AB620(D_800C7D7C[func_8001E6B8(5)]);
        }
    }
}
