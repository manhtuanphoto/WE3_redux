#include <stdint.h>

/* Scratchpad (0x1F800000) globals */
#define SPAD_FLAG_2A9         (*(volatile uint8_t *)0x1F8002A9)
#define SPAD_ACTIVE_OWNER_ID  (*(volatile uint8_t *)0x1F8002AA)
#define SPAD_FLAG_2AC         (*(volatile uint8_t *)0x1F8002AC)
#define SPAD_BALL_STATE       (*(volatile uint32_t *)0x1F8002B0)
#define SPAD_TEAM_POSS(team)  (*(volatile uint8_t *)(0x1F8002CF + (team)))
#define SPAD_BALL_PTR         (*(uint8_t * volatile *)0x1F8002F4)
#define SPAD_TOUCH_TIMER      (*(volatile int16_t *)0x1F80031C)
#define SPAD_SIDE_ID(side)    (*(volatile uint8_t *)(0x1F80031E + (side) * 2))

/* Player entity array: stride 0x3E8 (1000 bytes) */
extern uint8_t D_800FF748[];

extern uint8_t func_8005CE14(void);
extern uint8_t func_80052D90(void *player);
extern void func_80055AD4(void *player);
extern uint8_t func_8004E410(void *player);
extern int16_t func_8001E694(uint8_t angle, int32_t dist);
extern int16_t func_8001E5A4(uint8_t angle, int32_t dist);
extern int32_t func_8004E5A8(uint8_t angle);
extern void func_800951C4(void *player, uint8_t angle, int32_t arg2, uint32_t speed);
extern uint8_t func_80052C6C(void *player);
extern int32_t func_8001E6B8(int32_t range);

/*
 * Handles ball-player collision / first-touch / deflection physics when the
 * ball's speed (ball+0x3A0) exceeds a threshold determined by Scratchpad timer
 * 0x1F80031C and team possession (0x1F8002CF[team]).
 * Places the ball at distance 0x68 from the player along angle func_8004E410,
 * transfers momentum via func_800951C4, sets ball state 0x1F8002B0 = 0xB and
 * team possession, and optionally puts the player into trap/stumble state 0xE.
 * Returns 1 if touch/deflection occurred, 0 otherwise.
 */
int32_t func_8004DEFC(uint8_t *player) {
    uint32_t speed_thresh;
    int16_t timer;
    uint8_t *ball;
    uint8_t team_poss;
    uint8_t touch_angle;
    uint32_t out_speed;
    uint8_t angle_diff;
    int32_t extra_arg;

    if (SPAD_FLAG_2A9 != 0) {
        return 0;
    }

    team_poss = SPAD_TEAM_POSS(player[0x398]);
    if (team_poss == 0) {
        uint8_t pid = player[0x38D];
        if (pid == SPAD_ACTIVE_OWNER_ID) {
            return 0;
        }
        if (SPAD_BALL_STATE == 1) {
            if (pid == SPAD_SIDE_ID(player[0x399])) {
                return 0;
            }
            if (player[0x38D] == func_8005CE14()) {
                return 0;
            }
        }
        timer = SPAD_TOUCH_TIMER;
        if (timer >= 0x20) {
            return 0;
        }
        speed_thresh = (timer < 0x0D) ? 0x2800 : 0x9000;
    } else {
        timer = SPAD_TOUCH_TIMER;
        if (timer >= 0x20) {
            return 0;
        }
        if (player[0x38D] == SPAD_FLAG_2AC) {
            return 0;
        }
        if (timer < 0x11) {
            speed_thresh = 0x2800;
        } else {
            speed_thresh = 0x9000 - ((uint32_t)(0x20 - timer) * 3) * 0x100;
        }
    }

    ball = SPAD_BALL_PTR;
    if (speed_thresh >= *(uint32_t *)(ball + 0x3A0)) {
        return 0;
    }

    {
        uint8_t flag_2a9 = SPAD_FLAG_2A9;
        if (flag_2a9 != 0 && SPAD_TEAM_POSS(player[0x398]) == 1) {
            uint8_t idx = (uint8_t)(flag_2a9 % 24);
            uint8_t *other = D_800FF748 + (uint32_t)idx * 0x3E8;
            if (func_80052D90(other) == 0) {
                other[0x396] = 0x10;
                other[0x397] = 0;
            }
        }
    }

    if (SPAD_BALL_STATE == 4) {
        int16_t px = *(int16_t *)(player + 0x02);
        int32_t check_x = (player[0x399] != 0) ? -px : px;
        if (check_x < -0x23E1) {
            func_80055AD4(player);
        }
    }

    touch_angle = func_8004E410(player);

    ball = SPAD_BALL_PTR;
    angle_diff = (uint8_t)(player[0x3A4] - ball[0x3A4]);
    if (angle_diff >= 0x81) {
        angle_diff = (uint8_t)(ball[0x3A4] - player[0x3A4]);
    }

    if (SPAD_BALL_STATE == 0x0B) {
        SPAD_TOUCH_TIMER = 0x0C;
        out_speed = (*(uint32_t *)(player + 0x3A0) * (uint32_t)angle_diff) >> 9;
    } else {
        out_speed = (*(uint32_t *)(player + 0x3A0) * (uint32_t)angle_diff) >> 8;
        SPAD_TOUCH_TIMER = 0;
    }

    ball = SPAD_BALL_PTR;
    *(int16_t *)(ball + 0x02) = (int16_t)(*(int16_t *)(player + 0x02) + func_8001E694(touch_angle, 0x68));

    ball = SPAD_BALL_PTR;
    *(int16_t *)(ball + 0x06) = (int16_t)(*(int16_t *)(player + 0x06) + func_8001E5A4(touch_angle, 0x68));

    extra_arg = func_8004E5A8(touch_angle);
    func_800951C4(player, touch_angle, extra_arg, out_speed);

    SPAD_BALL_STATE = 0x0B;
    SPAD_TEAM_POSS(player[0x398]) = 1;

    ball = SPAD_BALL_PTR;
    ball[0x3B0] = 2;
    ball[0x3B2] = 2;

    if (func_80052C6C(player) == 0) {
        player[0x3D0] = (uint8_t)(func_8001E6B8(4) + 4);
        player[0x396] = 0x0E;
        player[0x397] = 0x40;
    }

    return 1;
}
