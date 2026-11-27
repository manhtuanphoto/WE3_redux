/*
 * func_8005ADE4 - Update team formation zone grid & role assignments
 *
 * Purpose:
 *   1) Copies the 3x10 tactical/zone grid from D_800C81F4[formation] into
 *      D_800E6C70[side] for both teams.
 *   2) Maps each active outfield player's role (+0x391) to its slot index in
 *      D_800FF748 + 0xABE3 + side*26.
 *   3) Adjusts defender/wingback/midfielder role IDs (21<->23, 22<->24/25) and
 *      updates D_800E6C70 from D_800C844C / D_800C8458 depending on formation
 *      (jtbl_80011F58) and tactical mentality (D_800FF748 + 0xABDC[side]).
 *
 * Arguments:
 *   None (void).
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Globals read & written:
 *   Scratch 0x2E0     (u8) - kickoff/home-side XOR flag
 *   Scratch 0x2FD[2]  (u8) - formation id (0..19) per side
 *   D_800E6C70[2][3][10]   - active tactical zone table (written)
 *   D_800C81F4[]           - ROM formation zone tables (30 bytes each)
 *   D_800C8118[]           - ROM formation role tables (11 bytes each)
 *   D_800C844C[], D_800C8458[] - tactical override sub-grids (3x4 and 3x3)
 *   D_800FF748 + 0xABDC[2] - team mentality/attacking flag (1 or 2 = active)
 *   D_800FF748 + 0xABE3[2][26] - role-to-slot lookup per side
 *   Actor +0x00       (u8) - active flag
 *   Actor +0x38D      (u8) - actor id (1 and 12 are GKs, skipped in loop 2)
 *   Actor +0x391      (u8) - formation role id (read/updated)
 *   Actor +0x398      (u8) - team side (0/1)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char D_800FF748[];
extern unsigned char D_800E6C70[];
extern unsigned char D_800C81F4[];
extern unsigned char D_800C8118[];
extern unsigned char D_800C844C[];
extern unsigned char D_800C8458[];

void func_8005ADE4(void)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char team;
    unsigned char row;
    unsigned char col;
    unsigned char idx;
    unsigned char *actor;

    /* Copy 3x10 formation zone grid for both sides */
    for (team = 0; team < 2; team++) {
        for (row = 0; row < 3; row++) {
            for (col = 0; col < 10; col++) {
                unsigned int form = B(scratch, 0x2FD + team);
                B(D_800E6C70, team * 30 + row * 10 + col) =
                    B(D_800C81F4, form * 30 + row * 10 + col);
            }
        }
    }

    /* Map outfield players' formation roles into D_800FF748 + 0xABE3 */
    actor = D_800FF748 + 1000;
    for (idx = 0; idx < 22; idx++, actor += 1000) {
        unsigned char side = (unsigned char)(B(scratch, 0x2E0) ^ (idx / 11U));
        unsigned char actor_id = B(actor, 0x38D);

        if (actor_id != 1 && actor_id != 12 && B(actor, 0) != 0) {
            unsigned char slot;
            unsigned char form = B(scratch, 0x2FD + side);

            for (slot = 0; slot < 11; slot++) {
                unsigned char role = B(D_800C8118, form * 11 + slot);
                if (B(actor, 0x391) == role) {
                    B(D_800FF748, 0xABE3 + side * 26 + role) = slot;
                    break;
                }
            }
        }
    }

    /* Adjust roles for attacking/defending mentality based on formation */
    for (team = 0; team < 2; team++) {
        unsigned char *first_outfield = (team == 0) ? (D_800FF748 + 0x7D0) : (D_800FF748 + 0x32C8);
        unsigned char side = B(first_outfield, 0x398);
        unsigned int form = B(scratch, 0x2FD + side);

        switch (form) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 11:
        case 12:
        case 13:
        case 16:
        case 17: {
            unsigned char mentality = B(D_800FF748, 0xABDC + side);
            if ((unsigned int)(mentality - 1) < 2U) {
                for (row = 0; row < 3; row++) {
                    for (col = 0; col < 4; col++) {
                        B(D_800E6C70, side * 30 + row * 10 + col) =
                            B(D_800C844C, row * 4 + col);
                    }
                }
                actor = first_outfield;
                for (idx = 0; idx < 10; idx++, actor += 1000) {
                    if (B(actor, 0x391) == 22) {
                        B(actor, 0x391) = (B(D_800FF748, 0xABDC + side) == 1) ? 24 : 25;
                        B(D_800FF748, 0xABE3 + side * 26 + B(actor, 0x391)) = 1;
                    }
                    if (B(actor, 0x391) == 21) {
                        B(actor, 0x391) = 23;
                        B(D_800FF748, 0xABE3 + side * 26 + 23) = 2;
                    }
                }
            } else {
                actor = first_outfield;
                for (idx = 0; idx < 10; idx++, actor += 1000) {
                    if ((unsigned int)(B(actor, 0x391) - 24) < 2U) {
                        B(actor, 0x391) = 22;
                        B(D_800FF748, 0xABE3 + side * 26 + 22) = 1;
                    }
                    if (B(actor, 0x391) == 23) {
                        B(actor, 0x391) = 21;
                        B(D_800FF748, 0xABE3 + side * 26 + 21) = 2;
                    }
                }
            }
            break;
        }

        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 14:
        case 15:
        case 18:
        case 19: {
            unsigned char mentality = B(D_800FF748, 0xABDC + side);
            if ((unsigned int)(mentality - 1) < 2U) {
                for (row = 0; row < 3; row++) {
                    for (col = 0; col < 3; col++) {
                        B(D_800E6C70, side * 30 + row * 10 + col) =
                            B(D_800C8458, row * 3 + col);
                    }
                }
                actor = first_outfield;
                for (idx = 0; idx < 10; idx++, actor += 1000) {
                    if (B(actor, 0x391) == 23) {
                        B(actor, 0x391) = (B(D_800FF748, 0xABDC + side) == 1) ? 24 : 25;
                        B(D_800FF748, 0xABE3 + side * 26 + B(actor, 0x391)) = 1;
                    }
                }
            } else {
                actor = first_outfield;
                for (idx = 0; idx < 10; idx++, actor += 1000) {
                    if ((unsigned int)(B(actor, 0x391) - 24) < 2U) {
                        B(actor, 0x391) = 23;
                        B(D_800FF748, 0xABE3 + side * 26 + 23) = 1;
                    }
                }
            }
            break;
        }
        }
    }
}
