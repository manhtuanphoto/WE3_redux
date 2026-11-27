#include <stdint.h>

/* Scratchpad (0x1F800000) globals */
#define SPAD_ACTION_MASK(side) (*(volatile uint16_t *)(0x1F800032 + (side) * 2))
#define SPAD_TARGET_POS_Y      (*(volatile int16_t *)0x1F8000F2)
#define SPAD_PAD_CUR(idx)      (*(volatile uint16_t *)(0x1F800230 + (idx) * 8))
#define SPAD_PAD_NEW(idx)      (*(volatile uint16_t *)(0x1F800234 + (idx) * 8))
#define SPAD_DPAD_DIR          (*(volatile uint8_t *)0x1F80028C)
#define SPAD_FLAG_2A9          (*(volatile uint8_t *)0x1F8002A9)
#define SPAD_TEAM_POSS(team)   (*(volatile uint8_t *)(0x1F8002CF + (team)))
#define SPAD_CTRL_MODE         (*(volatile uint8_t *)0x1F8002E2)
#define SPAD_CAM_ROT_MODE      (*(volatile uint8_t *)0x1F8002E7)
#define SPAD_TOUCH_TIMER       (*(volatile int16_t *)0x1F80031C)

/* Player attribute tables: team stride 0x108 (264), player stride 0x0C (12) */
extern uint8_t D_8010C32F[];
extern uint32_t D_8010C330[];

extern void func_800484F8(int16_t pos_x, int16_t pos_z, uint8_t arg2, int32_t arg3);

/*
 * Updates controller pad input (player+0x3D4 / 0x3D6) and camera-rotated D-pad
 * direction (0x1F80028C) for `player`. When input matches the action button mask
 * at 0x1F800032[team], resets player+0x3B2, computes target vertical offset/vel
 * at 0x1F8000F2 from player stats in D_8010C330/D_8010C32F, and calls
 * func_800484F8(pos_x, pos_z, player+0x3B4, 0).
 */
void func_8009406C(uint8_t *player) {
    uint8_t state;
    uint8_t team;
    uint8_t pad_idx;
    int32_t active_input_state = 0;

    if (SPAD_FLAG_2A9 == 0) {
        int32_t thresh = 0x17 - (int32_t)(uint8_t)(player[0x3B9] / 10);
        if (SPAD_TOUCH_TIMER < thresh) {
            return;
        }
    }

    state = player[0x396];
    team = player[0x398];

    if (state < 0x0C) {
        if (state >= 0x0A || state < 6 || state == 8) {
            active_input_state = 1;
        }
    } else {
        if (state == 0x81 || state == 0x0E || state == 0x95 || state == 0x97) {
            active_input_state = 1;
        }
    }

    if (active_input_state) {
        uint8_t ctrl = SPAD_CTRL_MODE;
        uint8_t rot;
        uint16_t cur_pad;

        pad_idx = team;
        if (ctrl == 0) {
            pad_idx = 0;
        } else if (ctrl == 2) {
            pad_idx = 1;
        }

        *(uint16_t *)(player + 0x3D4) = SPAD_PAD_CUR(pad_idx);
        *(uint16_t *)(player + 0x3D6) = SPAD_PAD_NEW(pad_idx);

        rot = SPAD_CAM_ROT_MODE;
        cur_pad = *(uint16_t *)(player + 0x3D4);
        if (rot == 0) {
            /* Rotate D-pad nibble (bits 12..15) left by 1 */
            SPAD_DPAD_DIR = (uint8_t)(((cur_pad & 0x7000) >> 11) | (cur_pad >> 15));
        } else if (rot == 0x80) {
            /* Rotate D-pad nibble (bits 12..15) right by 1 */
            SPAD_DPAD_DIR = (uint8_t)((cur_pad >> 13) | ((cur_pad & 0x1000) >> 9));
        } else {
            SPAD_DPAD_DIR = (uint8_t)(cur_pad >> 12);
        }
    } else {
        uint8_t skip_pad = 0;
        if (SPAD_FLAG_2A9 != 0) {
            if (SPAD_TEAM_POSS(team) == 0) {
                skip_pad = 1;
            }
        } else {
            int32_t thresh = 0x17 - (int32_t)(uint8_t)(player[0x3B9] / 10);
            if (SPAD_TOUCH_TIMER < thresh) {
                skip_pad = 1;
            }
        }

        if (!skip_pad) {
            uint8_t ctrl = SPAD_CTRL_MODE;
            uint16_t mask = SPAD_ACTION_MASK(team);
            if (ctrl == 0) {
                if (mask & SPAD_PAD_CUR(0)) {
                    *(uint16_t *)(player + 0x3D4) = mask;
                }
            } else if (ctrl == 1) {
                if (mask & SPAD_PAD_CUR(team)) {
                    *(uint16_t *)(player + 0x3D4) = mask;
                }
            } else if (ctrl == 2 || ctrl == 3) {
                if ((mask & SPAD_PAD_CUR(0)) || (mask & SPAD_PAD_CUR(1))) {
                    *(uint16_t *)(player + 0x3D4) = mask;
                }
            }
        }
    }

    if (*(uint16_t *)(player + 0x3D4) == SPAD_ACTION_MASK(player[0x398])) {
        uint32_t slot = player[0x392];
        uint32_t t = player[0x398];
        uint32_t attr_off = (t * 0x108) + (slot * 0x0C);
        uint32_t stat_hi = ((*(uint32_t *)((uint8_t *)D_8010C330 + attr_off) >> 11) & 0x7F) + 0x64;
        uint8_t stat_lo = D_8010C32F[attr_off] & 0x0F;
        int32_t scaled = -((int32_t)stat_hi * 736);

        player[0x3B2] = 0;
        SPAD_TARGET_POS_Y = (int16_t)((scaled / 180) - (int32_t)stat_lo + 0x46);

        func_800484F8(*(int16_t *)(player + 0x02), *(int16_t *)(player + 0x06), player[0x3B4], 0);
    }
}
