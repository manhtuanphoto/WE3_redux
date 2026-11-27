#include <stdint.h>

/* Scratchpad (0x1F800000) globals */
#define SPAD_ACTIVE_OWNER_ID (*(volatile uint8_t *)0x1F8002AA)
#define SPAD_BALL_STATE      (*(volatile uint32_t *)0x1F8002B0)
#define SPAD_BALL_PTR        (*(uint8_t * volatile *)0x1F8002F4)

extern void func_8005BD1C(void *player);
extern uint8_t func_8005B7CC(uint8_t target_dir, uint8_t cur_dir, int32_t step);
extern uint8_t func_8003D274(void *player, int32_t arg1, int32_t arg2);
extern void func_8003D99C(void *player, uint8_t arg1, int32_t arg2);
extern int32_t func_8001E6B8(int32_t range);
extern int32_t func_8002B1E4(void *player);
extern void func_8004B0F0(void *player, uint8_t arg1, int32_t arg2, int32_t arg3);
extern void func_800AB620(int32_t sfx_id);
extern void func_8002B5A4(void *player, int32_t arg1);
extern void func_8005707C(void *player);
extern void func_8005BA30(void *player, int32_t action, int32_t arg2);
extern int32_t func_8004E5A8(void *player);
extern void func_8009462C(void *player, int32_t arg1, uint8_t dir, int32_t arg3, int32_t arg4);
extern void func_800498AC(void *player);

/*
 * State handler for an acrobatic / jumping kick or volley sequence (substate
 * player->unk_397 in {0, 1, 2}):
 *   - 0: Turns player toward unk_3AB, advances substate if unk_390 != 0.
 *   - 1: Turns and integrates velocity (0x3CA, 0x3CC, 0x3CE); when unk_390 >= 8,
 *        tests ball contact via func_8003D274 and triggers kick/shot + SFX 0x507.
 *   - 2: Integrates X/Z, damps Y (*0.8) and XZ velocity (*0.9), clears 0x3CA
 *        (and 0x3CE, pos_y if unk_390 == 0xF), and on unk_390 == 0 flips facing
 *        by 0x80, enters action 0x21, and transitions to state 0x11.
 */
void func_8003A8E0(uint8_t *player) {
    uint8_t substate = player[0x397];
    uint8_t new_dir;
    int32_t facing_diff;

    switch (substate) {
        case 0:
            func_8005BD1C(player);
            new_dir = func_8005B7CC(player[0x3AB], player[0x3AA], 0x10);
            player[0x3AA] = new_dir;
            facing_diff = 0x1C0 - (int32_t)new_dir;
            *(int16_t *)(player + 0x26) = (int16_t)((facing_diff % 256) << 4);
            if (player[0x390] != 0) {
                player[0x397] = (uint8_t)(player[0x397] + 1);
            }
            break;

        case 1:
            func_8005BD1C(player);
            new_dir = func_8005B7CC(player[0x3AB], player[0x3AA], 0x10);
            player[0x3AA] = new_dir;
            facing_diff = 0x1C0 - (int32_t)new_dir;
            *(int16_t *)(player + 0x26) = (int16_t)((facing_diff % 256) << 4);

            *(uint16_t *)(player + 0x02) += *(uint16_t *)(player + 0x3CA);
            *(uint16_t *)(player + 0x04) += *(uint16_t *)(player + 0x3CC);
            *(uint16_t *)(player + 0x06) += *(uint16_t *)(player + 0x3CE);

            if (player[0x390] >= 8) {
                if (func_8003D274(player, 1, 1) != 0) {
                    int32_t rng_type;
                    int32_t p2, p3;
                    int32_t r0, r1;
                    uint8_t *ball;

                    func_8003D99C(player, player[0x3B2], 1);
                    rng_type = func_8001E6B8(8);

                    if (func_8001E6B8(3) != 0) {
                        p2 = func_8001E6B8(4) + 0xA8;
                        r0 = func_8001E6B8(0x0C);
                        r1 = func_8002B1E4(player);
                        p3 = r0 + r1 + 2;
                    } else {
                        p2 = func_8001E6B8(4) + 0xAC;
                        p3 = func_8001E6B8(4) - 0x0F;
                    }

                    func_8004B0F0(player, (uint8_t)rng_type, p2, p3);

                    r0 = func_8001E6B8(2);
                    ball = SPAD_BALL_PTR;
                    *(int16_t *)(ball + 0x3CA) = (int16_t)r0;

                    r0 = func_8001E6B8(3);
                    ball = SPAD_BALL_PTR;
                    *(int16_t *)(ball + 0x3C8) = (int16_t)(9 - r0);

                    func_800AB620(0x507);

                    ball = SPAD_BALL_PTR;
                    ball[0x396] = 4;
                    func_8002B5A4(player, 0x0A);
                    func_8005707C(player);
                }
                player[0x397] = (uint8_t)(player[0x397] + 1);
            }
            break;

        case 2: {
            int16_t pos_y;
            int16_t vx, vz;

            func_8005BD1C(player);
            pos_y = *(int16_t *)(player + 0x04);
            vx = *(int16_t *)(player + 0x3CA);
            vz = *(int16_t *)(player + 0x3CE);

            *(uint16_t *)(player + 0x02) += (uint16_t)vx;
            *(uint16_t *)(player + 0x06) += (uint16_t)vz;

            *(int16_t *)(player + 0x04) = (int16_t)((pos_y * 8) / 10);
            *(int16_t *)(player + 0x3CA) = (int16_t)((vx * 9) / 10);
            *(int16_t *)(player + 0x3CE) = (int16_t)((vz * 9) / 10);

            /* Delay-slot write in assembly zeroes 0x3CA unconditionally */
            *(int16_t *)(player + 0x3CA) = 0;
            if (player[0x390] == 0x0F) {
                *(int16_t *)(player + 0x3CE) = 0;
                *(int16_t *)(player + 0x04) = 0;
            }

            if (player[0x390] == 0) {
                uint8_t flipped_dir = (uint8_t)(player[0x3AA] + 0x80);
                player[0x3AA] = flipped_dir;
                facing_diff = 0x1C0 - (int32_t)flipped_dir;
                *(int16_t *)(player + 0x26) = (int16_t)((facing_diff % 256) << 4);

                func_8005BA30(player, 0x21, 0);
                player[0x3AD] = 0;
                player[0x396] = 0x11;
                player[0x397] = 0;
            }
            break;
        }

        default:
            break;
    }

    if (SPAD_BALL_STATE != 4 || player[0x38D] != SPAD_ACTIVE_OWNER_ID) {
        int32_t val = func_8004E5A8(player);
        func_8009462C(player, 0x0C, player[0x3AA], 4, val);
    }

    func_800498AC(player);
}
