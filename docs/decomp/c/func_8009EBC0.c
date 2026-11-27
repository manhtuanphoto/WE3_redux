#include <stdint.h>

/* Scratchpad (0x1F800000) references */
#define SPAD_TARGET_X         (*(volatile int16_t *)0x1F8000F8)
#define SPAD_TARGET_Z         (*(volatile int16_t *)0x1F8000FC)
#define SPAD_TEAM_POSS(team)  (*(volatile uint8_t *)(0x1F8002CF + (team)))
#define SPAD_BALL_PTR         (*(uint8_t * volatile *)0x1F8002F4)
#define SPAD_FLAG_2FC         (*(volatile uint8_t *)0x1F8002FC)
#define SPAD_DEF_LINE_X(side) (*(volatile int16_t *)(0x1F80030C + (side) * 2))
#define SPAD_OFF_LINE_X(side) (*(volatile int32_t *)(0x1F800314 + (side) * 4))

extern int32_t func_8009FEDC(void *player);
extern void func_8005A85C(void *player, volatile int16_t *out_x, volatile int16_t *out_z);
extern int16_t func_8005A76C(int16_t val, int32_t target, int32_t step);
extern int16_t func_8005A7E4(int16_t val, int32_t target, int32_t step);
extern void func_80068FC0(void *player, int16_t target_x, int16_t target_z);

/*
 * Computes an off-ball tactical target position (SPAD_TARGET_X, SPAD_TARGET_Z)
 * for `player` based on team possession, attacking side (0x399), and role (0x391),
 * applies cross-quadrant center-pitch avoidance if needed, and steers the player
 * toward the target via func_80068FC0.
 */
void func_8009EBC0(uint8_t *player) {
    int32_t attack_sign;
    uint8_t team_poss;
    uint8_t *ball;
    int32_t ball_z_sign;
    int32_t player_z_sign;
    int32_t player_x_sign;
    int32_t ball_x_sign;

    if (func_8009FEDC(player) != 0) {
        return;
    }

    func_8005A85C(player, &SPAD_TARGET_X, &SPAD_TARGET_Z);

    attack_sign = (player[0x399] != 0) ? -1 : 1;
    team_poss = SPAD_TEAM_POSS(player[0x398]);

    if (team_poss == 0) {
        /* Team does not have possession (defending) */
        uint8_t role = player[0x391];
        uint8_t idx = (uint8_t)(role - 6);

        SPAD_TARGET_X = (int16_t)(SPAD_TARGET_X + attack_sign * 0xB00);

        switch (idx) {
        case 0:
        case 1:
        case 3:
        case 4:
        case 5:
        case 6: {
            int32_t s_ball_z;
            int32_t s_tgt_z;

            ball = SPAD_BALL_PTR;
            s_ball_z = (*(int16_t *)(ball + 0x06) >= 0) ? 1 : -1;
            s_tgt_z = (SPAD_TARGET_Z >= 0) ? 1 : -1;
            if (s_ball_z == s_tgt_z) {
                SPAD_TARGET_Z = func_8005A76C(SPAD_TARGET_Z, 0, 3);
            }
            SPAD_TARGET_X = (int16_t)(((int32_t)SPAD_DEF_LINE_X(player[0x399]) + (int32_t)SPAD_TARGET_X) / 2);
            break;
        }
        case 2:
        case 7:
        case 8:
        case 9:
        case 10:
        default:
            SPAD_TARGET_X = (int16_t)(SPAD_TARGET_X + (attack_sign << 11));
            break;
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
            break;
        }
    } else {
        /* Team has possession (attacking) */
        uint8_t role = player[0x391];
        uint8_t idx = (uint8_t)(role - 9);

        SPAD_TARGET_X = (int16_t)(SPAD_TARGET_X - (((int32_t)team_poss + attack_sign) << 8));

        switch (idx) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
            SPAD_TARGET_X = (int16_t)((SPAD_OFF_LINE_X(player[0x399]) + (int32_t)SPAD_TARGET_X) / 2);
            break;
        case 5:
        case 6:
            SPAD_TARGET_Z = func_8005A76C(SPAD_TARGET_Z, 0, 4);
            /* fall through */
        case 7:
            SPAD_TARGET_X = (int16_t)((SPAD_OFF_LINE_X(player[0x399]) + (int32_t)SPAD_TARGET_X - attack_sign * 0x500) / 2);
            break;
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
            SPAD_TARGET_X = (int16_t)(SPAD_TARGET_X - attack_sign * 0x500);
            break;
        default:
            SPAD_TARGET_X = (int16_t)(SPAD_TARGET_X - attack_sign * 0x600);
            break;
        }
    }

    /* If player and ball are in diagonally opposite quadrants across (0,0), steer around center */
    ball = SPAD_BALL_PTR;
    ball_z_sign = (*(int16_t *)(ball + 0x06) >= 0) ? 1 : -1;
    player_z_sign = (*(int16_t *)(player + 0x06) >= 0) ? 1 : -1;
    if (ball_z_sign != player_z_sign) {
        player_x_sign = (*(int16_t *)(player + 0x02) >= 0) ? 1 : -1;
        ball_x_sign = (*(int16_t *)(ball + 0x02) >= 0) ? 1 : -1;
        if (player_x_sign != ball_x_sign) {
            int16_t px = *(int16_t *)(player + 0x02);
            int32_t abs_px = (px >= 0) ? px : -px;
            if (abs_px < 0x800) {
                int16_t tgt_x;
                int32_t avoid_x;
                SPAD_TARGET_Z = func_8005A7E4(SPAD_TARGET_Z, 0, 0x28);
                tgt_x = SPAD_TARGET_X;
                avoid_x = (tgt_x <= 0) ? 0x800 : -0x800;
                SPAD_TARGET_X = func_8005A7E4(tgt_x, avoid_x, 0x1E);
            }
        }
    }

    func_80068FC0(player, SPAD_TARGET_X, SPAD_TARGET_Z);

    if (SPAD_FLAG_2FC == 0) {
        player[0x3E2] = 0;
        player[0x3E3] = 0;
    }
}
