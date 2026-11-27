/*
 * func_8004ED68 - Update HUD player-cursor / jersey-number / tactic-bar sprites
 *
 * Purpose:
 *   Updates the active player cursor/number/position-label HUD primitives for
 *   either side 0 (a0 == 0) or side 1 (a0 != 0), plus the tactic-level bars
 *   and blinking substitution/strategy indicator via func_8001D004.
 *
 * Arguments:
 *   slot_sel (u8) - 0 for primary cursor (P1 / home-controlled), nonzero for secondary
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Actor fields read & written:
 *   Scratch 0x240/0x241 (u8)  - active controlled player actor IDs
 *   Scratch 0x290       (u32) - frame counter (bit 3 = 8-frame blink)
 *   Scratch 0x2E0       (u8)  - home/kickoff side selector
 *   Scratch 0x2E2       (u8)  - controller/cursor mode (2 = fixed P1/P2 slots)
 *   Scratch 0x2FD[2]    (u8)  - formation ID per side
 *   Scratch 0x2FF[2]    (u8)  - active tactic/strategy bar level (0..2)
 *   Scratch 0x305[2]    (u8)  - blinking indicator state per side
 *   Actor +0x38D        (u8)  - actor ID
 *   Actor +0x392        (u8)  - team slot (0..10)
 *   Actor +0x398        (u8)  - team side (0/1)
 *   Actor +0x399        (u8)  - attack direction (0/1)
 *   D_800FF748 + 0x8E9C (u8[2][22]) - lineup slot table per side
 *   D_800CA30C[]        (u8)  - formation position-label UV index table (stride 11)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   Frame bit 0x08 = toggles every 8 frames (~0.267 s) for HUD blink.
 *   Sprite primitives 6, 39 (0x27), and 0/1 obtained from func_8001D004.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_800CA30C[];

extern unsigned char *func_8001D004(int prim_id);

void func_8004ED68(unsigned int slot_sel)
{
    unsigned char *prim = func_8001D004(6);
    unsigned int target_id;
    unsigned char *actor;
    unsigned char side;
    unsigned char atk_dir;
    unsigned int team_slot;
    unsigned char pos_idx;
    unsigned char *lineup;
    unsigned int div11;
    unsigned char uv_idx;
    int prim_mode;
    int i;

    if ((slot_sel & 0xFF) == 0) {
        if (B(Scratchpad, 0x2E2) == 2) {
            target_id = B(Scratchpad, 0x240);
        } else {
            target_id = B(Scratchpad, 0x240 + B(Scratchpad, 0x2E0));
        }

        actor = D_800FF748 + (target_id % 24) * 1000;
        side = B(actor, 0x398);
        atk_dir = B(actor, 0x399);
        team_slot = B(actor, 0x392);
        pos_idx = (unsigned char)(B(actor, 0x38D) - (atk_dir * 11 + 1));

        lineup = D_800FF748 + 0x8E9C + side * 22;
        for (i = 0; i < 11; i++) {
            if (B(lineup, i) == team_slot) {
                pos_idx = (unsigned char)i;
                break;
            }
        }

        div11 = team_slot / 11;
        B(prim, 3) = (unsigned char)((side << 7) + (div11 << 6));
        B(prim, 4) = (unsigned char)((team_slot % 11) << 4);

        prim = func_8001D004(0x27);
        uv_idx = B(D_800CA30C, B(Scratchpad, 0x2FD + side) * 11 + pos_idx);
        B(prim, 3) = (unsigned char)(((uv_idx & 1) << 4) - 0x80);
        B(prim, 4) = (unsigned char)(((uv_idx >> 1) << 4) - 0x70);
        prim_mode = 0;
    } else {
        if (B(Scratchpad, 0x2E2) == 2) {
            target_id = B(Scratchpad, 0x241);
        } else {
            target_id = B(Scratchpad, 0x240 + (B(Scratchpad, 0x2E0) ^ 1));
        }

        actor = D_800FF748 + (target_id % 24) * 1000;
        side = B(actor, 0x398);
        atk_dir = B(actor, 0x399);
        team_slot = B(actor, 0x392);
        pos_idx = (unsigned char)(B(actor, 0x38D) - (atk_dir * 11 + 1));

        lineup = D_800FF748 + 0x8E9C + side * 22;
        for (i = 0; i < 11; i++) {
            if (B(lineup, i) == team_slot) {
                pos_idx = (unsigned char)i;
                break;
            }
        }

        div11 = team_slot / 11;
        B(prim, 0x0F) = (unsigned char)((side << 7) + (div11 << 6));
        B(prim, 0x10) = (unsigned char)((team_slot % 11) << 4);

        prim = func_8001D004(0x27);
        uv_idx = B(D_800CA30C, B(Scratchpad, 0x2FD + side) * 11 + pos_idx);
        B(prim, 0x0F) = (unsigned char)(((uv_idx & 1) << 4) - 0x80);
        B(prim, 0x10) = (unsigned char)(((uv_idx >> 1) << 4) - 0x70);
        prim_mode = 1;
    }

    prim = func_8001D004(prim_mode);
    if ((W(Scratchpad, 0x290) & 8) != 0) {
        unsigned char ind = B(Scratchpad, 0x305 + side);
        if (ind != 0) {
            B(prim, (5 - ind) * 12 + 0x0A) = 9;
            B(prim, ((2 - ind) ^ 1) * 12 + 0x2E) = 0x5B;
        } else {
            B(prim, 0x2E) = 0x5B;
            B(prim, 0x3A) = 0x5B;
        }
    }

    for (i = 0; i < 3; i++) {
        if (i == B(Scratchpad, 0x2FF + side)) {
            B(prim, i * 12 + 0x0A) = 0x79;
        } else {
            B(prim, i * 12 + 0x0A) = 0x5B;
        }
    }
}
