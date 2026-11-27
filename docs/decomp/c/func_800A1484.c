#include <stdint.h>

/* Scratchpad (0x1F800000) references */
#define SPAD_ACTIVE_PLAYER_0 (*(volatile uint8_t *)0x1F800006)
#define SPAD_ACTIVE_PLAYER_1 (*(volatile uint8_t *)0x1F800007)

/* Player entity array: stride 0x3E8 (1000 bytes) per player */
extern uint8_t D_800FF748[];

/* Player attribute table: team stride 0x108 (264 bytes), player stride 0x0C (12 bytes) */
extern uint32_t D_8010C328[];

/* Jump table for player->unk_391 in [9..25] */
extern void *jtbl_80014584[17];

extern uint8_t func_80052D90(void *player);
extern int32_t func_8009FD40(void *target);
extern int32_t func_8009FD84(void *target);
extern void func_800A0F30(void *player, uint8_t target_id, int32_t arg2, int32_t arg3);

/*
 * Scans the 10 opposing outfield players (slots 11..2 or 22..13 depending on side)
 * to select a marking/pressing target for `player`, filtering by position/role,
 * active-player exclusion, Z-range, distance threshold, and avoiding duplicate marks.
 *
 * Known player struct offsets (stride 0x3E8):
 *   +0x000 (u8)  active flag
 *   +0x002 (s16) pos_x
 *   +0x006 (s16) pos_z
 *   +0x38D (u8)  player_id
 *   +0x391 (u8)  role / position
 *   +0x392 (u8)  slot_index (in team)
 *   +0x398 (u8)  team_index
 *   +0x399 (u8)  side (0 or 1)
 *   +0x3E0 (u8)  unk_3E0 (status; 0 or 2 skipped)
 *   +0x3E1 (u8)  mark_target_id
 *   +0x3E2 (u8)  unk_3E2 (set to 0xF when target acquired)
 *   +0x3E3 (u8)  unk_3E3 (cleared to 0 when target acquired)
 */
void func_800A1484(uint8_t *player, int32_t arg1, int32_t arg2, uint32_t base_radius) {
    uint8_t opp_base;
    uint8_t own_base;
    uint8_t side;
    uint32_t stat_top4;
    uint16_t radius;
    uint8_t *opp;
    uint8_t i;

    if (func_80052D90(player) != 0) {
        return;
    }

    /* Compute player stat bits [31:28] from D_8010C328[team * 0x108 + slot * 0x0C] */
    {
        uint32_t slot = player[0x392];
        uint32_t team = player[0x398];
        uint32_t offset = (team * 0x108) + (slot * 0x0C);
        stat_top4 = *(uint32_t *)((uint8_t *)D_8010C328 + offset) >> 28;
    }
    radius = (uint16_t)(base_radius + stat_top4 * 80);

    side = player[0x399];
    opp_base = (side == 1) ? 2 : 0x0D;
    own_base = (side == 0) ? 2 : 0x0D;

    /* Iterate backwards from opponent index (opp_base + 9) down to opp_base */
    opp = D_800FF748 + ((uint32_t)opp_base + 9) * 0x3E8;
    for (i = 0; i < 10; i++, opp -= 0x3E8) {
        uint8_t opp_status = opp[0x3E0];
        uint8_t role;
        uint8_t idx;
        int32_t check_z_and_dist = 0;
        int32_t check_dist_only = 0;
        int32_t already_marked = 0;

        if (opp_status == 0 || opp[0x000] == 0) {
            continue;
        }

        role = player[0x391];
        idx = (uint8_t)(role - 9);
        if (idx >= 17) {
            check_dist_only = 1;
        } else {
            /*
             * UNCLEAR: jtbl_80014584[idx] dispatch for player->unk_391 in [9..25].
             * The jump table data is external to this assembly unit; the 6 possible
             * target behaviors from the switch are documented below:
             *
             * Target A (.L800A15C0):
             *   if (opp[0x3E0] == 2 || opp[0x391] == 5) continue;
             *   if (func_8009FD40(opp) != 0) continue;
             *   check_dist_only = 1;
             *
             * Target B (.L800A15F8):
             *   if (opp[0x3E0] == 2 || opp[0x391] == 5) continue;
             *   if (func_8009FD84(opp) != 0) continue;
             *   check_dist_only = 1;
             *
             * Target C (.L800A1630):
             *   if (opp[0x3E0] == 2) continue;
             *   if (opp[0x38D] == SPAD_ACTIVE_PLAYER_0 || opp[0x38D] == SPAD_ACTIVE_PLAYER_1) continue;
             *   if (func_8009FD84(opp) != 0) continue;
             *   check_z_and_dist = 1;
             *
             * Target D (.L800A167C):
             *   if (opp[0x3E0] == 2) continue;
             *   if (opp[0x38D] == SPAD_ACTIVE_PLAYER_0 || opp[0x38D] == SPAD_ACTIVE_PLAYER_1) continue;
             *   if (func_8009FD40(opp) != 0) continue;
             *   check_z_and_dist = 1;
             *
             * Target E (.L800A16D0):
             *   if (opp[0x3E0] == 2) continue;
             *   if (opp[0x38D] == SPAD_ACTIVE_PLAYER_0 || opp[0x38D] == SPAD_ACTIVE_PLAYER_1) continue;
             *   check_z_and_dist = 1;
             *
             * Target F (.L800A1724):
             *   check_dist_only = 1;
             */
            (void)jtbl_80014584[idx];
            check_dist_only = 1;
        }

        if (check_z_and_dist) {
            int16_t opp_z = *(int16_t *)(opp + 0x06);
            int32_t abs_z = (opp_z >= 0) ? opp_z : -opp_z;
            if (abs_z >= 0x8B7) {
                continue;
            }
            check_dist_only = 1;
        }

        if (check_dist_only) {
            int32_t dx = *(int16_t *)(player + 0x02) - *(int16_t *)(opp + 0x02);
            int32_t dz = *(int16_t *)(player + 0x06) - *(int16_t *)(opp + 0x06);
            uint16_t dist_sq16 = (uint16_t)(dx * dx + dz * dz);
            int32_t rad_sq = (int32_t)radius * (int32_t)radius;

            if ((int32_t)dist_sq16 >= rad_sq) {
                continue;
            }

            /* Check if another active player in own_base..own_base+9 already marks opp */
            {
                uint8_t start_slot = (uint8_t)(own_base % 24);
                uint8_t *cand = D_800FF748 + (uint32_t)start_slot * 0x3E8;
                uint8_t j;

                for (j = 0; j < 10; j++, cand += 0x3E8) {
                    if (cand != opp && cand[0x000] != 0) {
                        if (cand[0x3E1] == opp[0x38D]) {
                            already_marked = 1;
                            break;
                        }
                    }
                }
            }

            if (already_marked) {
                continue;
            }

            player[0x3E2] = 0x0F;
            player[0x3E3] = 0;
            player[0x3E1] = opp[0x38D];

            {
                uint32_t slot = player[0x392];
                uint32_t team = player[0x398];
                uint32_t offset = (team * 0x108) + (slot * 0x0C);
                uint32_t stat = *(uint32_t *)((uint8_t *)D_8010C328 + offset) >> 28;
                if ((int32_t)(stat * 10) >= 60) {
                    func_800A0F30(player, player[0x3E1], arg1, arg2);
                    return;
                }
            }
        }
    }
}
