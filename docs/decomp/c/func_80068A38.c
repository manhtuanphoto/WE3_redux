#include <stdint.h>

/* Scratchpad (0x1F800000) globals */
#define SPAD_EXCLUDED_ID      (*(volatile uint8_t *)0x1F8002AD)
#define SPAD_MODE_2E1         (*(volatile uint8_t *)0x1F8002E1)
#define SPAD_BALL_PTR         (*(uint8_t * volatile *)0x1F8002F4)

/* Player entity array: stride 0x3E8 (1000 bytes) */
extern uint8_t D_800FF748[];

/* Team tactics/formation data */
extern uint8_t D_8010C1D8[];

/*
 * Builds a prioritized list of up to 10 outfield player IDs for `side`
 * (slots 2..11 if side == 0, or 13..22 if side != 0) into `out_ids[10]`,
 * padded with 0xFF.
 * First sorts active players by squared distance to the ball in descending
 * order (farthest first), then emits them grouped by role (player+0x391):
 *   1. Midfielders (6 <= role < 17)
 *   2. Defenders   (1 <= role < 6), skipped if D_8010C1D8[team + 0xD] == 6
 *   3. Forwards    (17 <= role < 25)
 * When Scratchpad 0x1F8002E1 == 5, player ID 0x1F8002AD is excluded.
 */
void func_80068A38(uint8_t *out_ids, uint8_t side) {
    uint8_t base_slot = (side == 0) ? 2 : 0x0D;
    uint8_t sorted_slots[10];
    uint32_t dist_sq[10];
    uint8_t *ball;
    int16_t ball_x, ball_z;
    uint8_t *p;
    uint8_t i, j, k;
    uint8_t out_count = 0;

    for (i = 0; i < 10; i++) {
        sorted_slots[i] = (uint8_t)(base_slot + i);
        dist_sq[i] = 0;
    }

    ball = SPAD_BALL_PTR;
    ball_x = *(int16_t *)(ball + 0x02);
    ball_z = *(int16_t *)(ball + 0x06);

    p = D_800FF748 + (uint32_t)base_slot * 0x3E8;
    for (i = 0; i < 10; i++, p += 0x3E8) {
        int32_t dx, dz;
        uint32_t d2;

        if (p[0x000] == 0) {
            continue;
        }

        dx = (int32_t)ball_x - (int32_t)(*(int16_t *)(p + 0x02));
        dz = (int32_t)ball_z - (int32_t)(*(int16_t *)(p + 0x06));
        d2 = (uint32_t)(dx * dx + dz * dz);

        for (j = 0; j < 10; j++) {
            if (dist_sq[j] < d2) {
                for (k = 9; k > j; k--) {
                    sorted_slots[k] = sorted_slots[k - 1];
                    dist_sq[k] = dist_sq[k - 1];
                }
                sorted_slots[j] = p[0x38D];
                dist_sq[j] = d2;
                break;
            }
        }
    }

    /* Pass 1: role in [6..16] */
    for (i = 0; i < 10; i++) {
        uint8_t *cand = D_800FF748 + (uint32_t)sorted_slots[i] * 0x3E8;
        uint8_t role;
        if (cand[0x000] == 0) {
            continue;
        }
        if (SPAD_MODE_2E1 == 5 && cand[0x38D] == SPAD_EXCLUDED_ID) {
            continue;
        }
        role = cand[0x391];
        if (role >= 6 && role < 0x11) {
            out_ids[out_count++] = cand[0x38D];
        }
    }

    /* Pass 2: role in [1..5], unless team formation flag D_8010C1D8[team + 0xD] == 6 */
    {
        uint8_t *first_p = D_800FF748 + (uint32_t)base_slot * 0x3E8;
        if (D_8010C1D8[first_p[0x398] + 0x0D] != 6) {
            for (i = 0; i < 10; i++) {
                uint8_t *cand = D_800FF748 + (uint32_t)sorted_slots[i] * 0x3E8;
                uint8_t role;
                if (cand[0x000] == 0) {
                    continue;
                }
                if (SPAD_MODE_2E1 == 5 && cand[0x38D] == SPAD_EXCLUDED_ID) {
                    continue;
                }
                role = cand[0x391];
                if (role >= 1 && role < 6) {
                    out_ids[out_count++] = cand[0x38D];
                }
            }
        }
    }

    /* Pass 3: role in [17..24] */
    for (i = 0; i < 10; i++) {
        uint8_t *cand = D_800FF748 + (uint32_t)sorted_slots[i] * 0x3E8;
        uint8_t role;
        if (cand[0x000] == 0) {
            continue;
        }
        if (SPAD_MODE_2E1 == 5 && cand[0x38D] == SPAD_EXCLUDED_ID) {
            continue;
        }
        role = cand[0x391];
        if (role >= 0x11 && role < 0x19) {
            out_ids[out_count++] = cand[0x38D];
        }
    }

    /* Pad unused slots with 0xFF */
    while (out_count < 10) {
        out_ids[out_count++] = 0xFF;
    }
}
