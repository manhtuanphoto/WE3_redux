#include <stdint.h>

/* Scratchpad (0x1F800000) globals */
#define SPAD_BALL_POS_X       (*(volatile int16_t *)0x1F800046)
#define SPAD_BALL_POS_Y       (*(volatile int16_t *)0x1F80005E)
#define SPAD_BALL_POS_Z       (*(volatile int16_t *)0x1F800076)
#define SPAD_TARGET_POS_X     (*(volatile int16_t *)0x1F8000F0)
#define SPAD_TARGET_POS_Y     (*(volatile int16_t *)0x1F8000F2)
#define SPAD_TARGET_POS_Z     (*(volatile int16_t *)0x1F8000F4)
#define SPAD_BALL_PTR         (*(uint8_t * volatile *)0x1F8002F4)

/* Player attribute tables: team stride 0x108 (264), player stride 0x0C (12) */
extern uint8_t D_8010C32F[];
extern uint32_t D_8010C330[];

extern int32_t func_8003D4E8(void *player, int32_t arg1);
extern void func_8005C2AC(void *player, int32_t action, int32_t arg2, uint8_t dir, uint32_t flag);
extern uint8_t func_8005B36C(int16_t x0, int16_t z0, int16_t x1, int16_t z1);
extern uint32_t func_800B1128(int32_t val);
extern int16_t func_8001E694(uint8_t angle, int32_t speed);
extern uint32_t func_800494C0(void *player);
extern uint8_t func_8001E730(int32_t val);
extern void func_8005C454(int16_t *vel_y, int32_t arg1, int16_t arg2);
extern uint8_t func_8003CEE4(void *player, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4);
extern void func_8005BA30(void *player, int32_t action, int32_t arg2);
extern uint8_t func_8005B7CC(uint8_t target_dir, uint8_t cur_dir, int32_t step, uint32_t flag);

/*
 * Attempts a jumping header / aerial contest action (action ID 0x24) when
 * ball+0x0C >= 0. Computes a 6-frame intercept trajectory to Scratchpad ball
 * coordinates (0x1F800046, 0x5E, 0x76), caps speed by player stat & turn angle,
 * adjusts vertical impulse via func_8005C454, validates via func_8003CEE4, and
 * initiates action 0x24.
 * Returns 1 if triggered, 0 otherwise.
 */
int32_t func_80035A5C(uint8_t *player) {
    uint8_t *ball = SPAD_BALL_PTR;
    uint32_t slot, team, attr_off;
    uint8_t stat_nibble;
    int32_t s6;
    int32_t dir_raw;
    uint8_t angle;
    int32_t dx, dz, dy;
    uint32_t dist;
    int32_t speed;
    int32_t speed_cap;
    uint8_t diff;
    int32_t y_param;
    const int32_t frames = 6;
    uint32_t stat_flag;
    uint8_t new_dir;
    int32_t facing_diff;

    if (*(int32_t *)(ball + 0x0C) < 0) {
        return 0;
    }

    slot = player[0x392];
    team = player[0x398];
    attr_off = (team * 0x108) + (slot * 0x0C);
    stat_nibble = D_8010C32F[attr_off] & 0x0F;

    dir_raw = func_8003D4E8(player, 9);
    func_8005C2AC(player, 0x24, 0, (uint8_t)dir_raw, player[0x3AC]);

    s6 = (int32_t)stat_nibble - 6;

    angle = func_8005B36C(SPAD_TARGET_POS_X, SPAD_TARGET_POS_Z, SPAD_BALL_POS_X, SPAD_BALL_POS_Z);

    dx = (int32_t)SPAD_TARGET_POS_X - (int32_t)SPAD_BALL_POS_X;
    dz = (int32_t)SPAD_TARGET_POS_Z - (int32_t)SPAD_BALL_POS_Z;
    dist = func_800B1128(dx * dx + dz * dz);
    speed = (int32_t)(dist / (uint32_t)frames);

    diff = (uint8_t)(player[0x3A4] - angle);
    if (diff >= 0x81) {
        diff = (uint8_t)(angle - player[0x3A4]);
    }
    speed_cap = ((int32_t)stat_nibble + 0x1A) - ((int32_t)diff >> 4);
    if (speed_cap < speed) {
        speed = speed_cap;
    }

    /* Note: assembly calls func_8001E694 for both 0x3CA and 0x3CE */
    *(int16_t *)(player + 0x3CA) = func_8001E694(angle, speed);
    *(int16_t *)(player + 0x3CE) = func_8001E694(angle, speed);

    dy = (int32_t)SPAD_BALL_POS_Y - (int32_t)SPAD_TARGET_POS_Y;
    *(int16_t *)(player + 0x3CC) = (int16_t)(dy / frames);

    if (func_800494C0(player) > 0xC3FF || func_8001E730((int32_t)player[0x3B8] - 0x3C) != 0) {
        y_param = -s6 - 4;
    } else {
        y_param = -s6;
    }
    func_8005C454((int16_t *)(player + 0x3CC), 0x14, (int16_t)y_param);

    SPAD_TARGET_POS_X = (int16_t)((uint16_t)SPAD_TARGET_POS_X + (*(int16_t *)(player + 0x3CA) * frames));
    SPAD_TARGET_POS_Y = (int16_t)((uint16_t)SPAD_TARGET_POS_Y + (*(int16_t *)(player + 0x3CC) * frames));
    SPAD_TARGET_POS_Z = (int16_t)((uint16_t)SPAD_TARGET_POS_Z + (*(int16_t *)(player + 0x3CE) * frames));

    if (func_8003CEE4(player, 8, 1, 1, 0x14400) == 0) {
        return 0;
    }

    func_8005BA30(player, 0x24, 0);
    player[0x3D1] = 0;

    slot = player[0x392];
    team = player[0x398];
    attr_off = (team * 0x108) + (slot * 0x0C);
    stat_flag = (*(uint32_t *)((uint8_t *)D_8010C330 + attr_off) >> 4) & 1;
    player[0x3AC] = (uint8_t)stat_flag;

    new_dir = func_8005B7CC((uint8_t)dir_raw, player[0x3AA], 0x0C, stat_flag);
    player[0x3AA] = new_dir;
    player[0x3A4] = angle;

    facing_diff = 0x1C0 - (int32_t)player[0x3AA];
    *(int16_t *)(player + 0x26) = (int16_t)((facing_diff % 256) << 4);
    player[0x3AB] = (uint8_t)dir_raw;
    player[0x3B2] = (uint8_t)dir_raw;

    return 1;
}
