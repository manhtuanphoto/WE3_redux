/*
 * func_800573EC
 *
 * Purpose:
 *   Triggers match commentary / audio callouts (func_800AB620) based on match
 *   timer D_800E6B84, score difference (SCRATCH+0x2D1[side] - [side^1]),
 *   shot/event type D_800FF748+0xAC4F (dispatched via jtbl_80011EDC), and the
 *   player's preferred/striking foot (D_8010C330 bit 4 vs actor+0x3AC).
 *   Increments counter D_800FF748+0xAC66 on fallback/non-terminal paths.
 *
 * Arguments:
 *   UNCLEAR: In the assembly, $s0 is saved and then dereferenced directly
 *   (0x392($s0), 0x398($s0), 0x3AC($s0)) without being loaded from $a0.
 *   Listed as `actor` here assuming caller-preserved $s0 (custom/split ABI).
 *
 * Return value:
 *   void
 *
 * Globals / Scratchpad / Actor fields read & written:
 *   D_800E6B84 (s16, R): match timer/countdown (0 triggers 0xD23..0xD27 callout)
 *   D_8010A323 (u8,  R): callout override flag
 *   D_8010A3AE (u8,  R): low 2 bits select 0xD24..0xD27 callout
 *   SCRATCH 0x2D1[2] (u8[2], R): per-side score (goals)
 *   D_800FF748 + 0xAC4F (u8, R): event/shot type index (0..12 into jtbl_80011EDC)
 *   D_800FF748 + 0xAC66 (u8, R/W): incremented by 1 on fallback path
 *   D_8010C330[side*264 + slot*12] (u32, R): bit 4 = preferred foot
 *   D_800C7D64[] / D_800C7D68[] (u16[], R): commentary ID lookup tables
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned char  D_800FF748[];
extern unsigned char  D_8010C330[];
extern signed short   D_800E6B84;
extern unsigned char  D_8010A323;
extern unsigned char  D_8010A3AE;
extern unsigned short D_800C7D64[];
extern unsigned short D_800C7D68[];
extern void          *jtbl_80011EDC[];

extern void         func_800AB620(unsigned int sfx_id);
extern unsigned int func_8001E6B8(unsigned int n);

void func_800573EC(unsigned char *actor /* UNCLEAR: passed in $s0 in asm */) {
    unsigned int side;
    int goal_diff;
    unsigned char evt;

    if (D_800E6B84 == 0) {
        if (D_8010A323 != 0) {
            func_800AB620(0xD23);
            return;
        }
        if (func_8001E6B8(4) != 0) {
            func_800AB620(0xD23);
        } else {
            func_800AB620((D_8010A3AE & 3u) | 0xD24u);
        }
    }

    side = B(actor, 0x398);
    goal_diff = (int)B(SCRATCH, 0x2D1u + side) - (int)B(SCRATCH, 0x2D1u + (side ^ 1u));
    if (goal_diff < 0) {
        B(D_800FF748, 0xAC66) = (unsigned char)(B(D_800FF748, 0xAC66) + 1);
        return;
    }

    evt = B(D_800FF748, 0xAC4F);
    if (evt < 0x0Du) {
        /*
         * UNCLEAR: dispatches via jump table jtbl_80011EDC[evt] (13 entries)
         * to one of the 7 blocks below; exact index-to-block mapping is
         * external to the .s file.
         */
        (void)jtbl_80011EDC;

        if (0) {
            /* Block .L800574C8 */
            if (func_8001E6B8(3) != 0) {
                func_800AB620(0xCD3);
            }
        } else if (0) {
            /* Block .L800574E8 */
            if (func_8001E6B8(4) == 0) {
                unsigned int stat_off = (unsigned int)B(actor, 0x392) * 12u + (unsigned int)B(actor, 0x398) * 264u;
                unsigned int pref_foot = (W(D_8010C330, stat_off) >> 4) & 1u;
                unsigned char used_foot = B(actor, 0x3AC);

                if (pref_foot == 0 && used_foot == 0) {
                    func_800AB620(0xCDA);
                } else if (pref_foot == 1 && used_foot == 1) {
                    func_800AB620(0xCDE);
                }
            }
        } else if (0) {
            /* Block .L800575C4 */
            if (func_8001E6B8(4) == 0) {
                unsigned int stat_off = (unsigned int)B(actor, 0x392) * 12u + (unsigned int)B(actor, 0x398) * 264u;
                unsigned int pref_foot = (W(D_8010C330, stat_off) >> 4) & 1u;
                unsigned char used_foot = B(actor, 0x3AC);

                if (pref_foot == 0 && used_foot == 0) {
                    func_800AB620(0xCDB);
                } else if (pref_foot == 1 && used_foot == 1) {
                    func_800AB620(0xCDC);
                }
            } else if (func_8001E6B8(2) == 0) {
                func_800AB620(0xCDD);
            }
        } else if (0) {
            /* Block .L800576C0 */
            if (func_8001E6B8(2) == 0) {
                func_800AB620(D_800C7D64[func_8001E6B8(2)]);
                return;
            }
        } else if (0) {
            /* Block .L800576F8 */
            if (func_8001E6B8(2) == 0) {
                func_800AB620(0xCEC);
                return;
            }
        } else if (0) {
            /* Block .L80057718 */
            if (func_8001E6B8(8) == 0) {
                func_800AB620(D_800C7D68[func_8001E6B8(2)]);
                return;
            }
        }
    }

    /* .L80057750 */
    B(D_800FF748, 0xAC66) = (unsigned char)(B(D_800FF748, 0xAC66) + 1);
}
