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

extern uint8_t func_80052A68(void *player, int32_t arg1, int32_t arg2);
extern int32_t func_8003D4E8(void *player, int32_t arg1);
extern void func_8005C2AC(void *player, int32_t action, int32_t arg2, uint8_t dir, uint32_t flag);
extern uint32_t func_800B1128(int32_t val); /* integer sqrt */
extern uint8_t func_8005B36C(int16_t x0, int16_t z0, int16_t x1, int16_t z1); /* atan2 */
extern int16_t func_8001E694(uint8_t angle, int32_t speed); /* cos * speed */
extern int16_t func_8001E5A4(uint8_t angle, int32_t speed); /* sin * speed */
extern void func_8005C454(int16_t *vel_y, int32_t arg1, int32_t arg2);
extern uint8_t func_8003CEE4(void *player, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4);
extern void func_8005BA30(void *player, int32_t action, int32_t arg2);
extern uint8_t func_8005B7CC(uint8_t target_dir, uint8_t cur_dir, int32_t step, uint32_t flag);

/*
 * Attempts an aerial / jumping action (action ID 0x2C) when the ball's vertical
 * state at ball+0x0C >= 0xB80. Computes a 6-frame intercept trajectory toward the
 * ball (clamped by player attribute D_8010C32F & 0xF + 0x20), updates Scratchpad
 * target coordinates (0x1F8000F0..F4), validates with func_8003CEE4, and applies
 * facing angle and action state.
 * Returns 1 if triggered, 0 otherwise.
 */
int32_t func_80039DD0(uint8_t *player) {
    uint8_t *ball = SPAD_BALL_PTR;
    int32_t dir_raw;
    uint8_t dir_u8;
    uint32_t team;
    uint32_t slot;
    uint32_t attr_off;
    uint32_t stat_flag;
    uint8_t stat_nibble;
    int32_t dx, dz, dy;
    uint32_t dist;
    uint32_t speed;
    uint32_t max_speed;
    uint8_t angle;
    int16_t vx, vy, vz;
    const int32_t frames = 6;
    uint8_t new_dir;
    int32_t facing_diff;

    if (*(int32_t *)(ball + 0x0C) < 0xB80) {
        return 0;
    }

    if (func_80052A68(player, 8, 0x24000) != 0) {
        return 0;
    }

    dir_raw = func_8003D4E8(player, 9) - 0x80;
    dir_u8 = (uint8_t)dir_raw;

    slot = player[0x392];
    team = player[0x398];
    attr_off = (team * 0x108) + (slot * 0x0C);
    stat_flag = (*(uint32_t *)((uint8_t *)D_8010C330 + attr_off) >> 4) & 1;

    func_8005C2AC(player, 0x2C, 0, dir_u8, stat_flag);

    dx = (int32_t)SPAD_TARGET_POS_X - (int32_t)SPAD_BALL_POS_X;
    dz = (int32_t)SPAD_TARGET_POS_Z - (int32_t)SPAD_BALL_POS_Z;
    dist = func_800B1128(dx * dx + dz * dz);
    speed = dist / (uint32_t)frames;

    slot = player[0x392];
    team = player[0x398];
    attr_off = (team * 0x108) + (slot * 0x0C);
    stat_nibble = D_8010C32F[attr_off] & 0x0F;
    max_speed = (uint32_t)stat_nibble + 0x20;
    if (max_speed < speed) {
        speed = max_speed;
    }

    angle = func_8005B36C(SPAD_TARGET_POS_X, SPAD_TARGET_POS_Z, SPAD_BALL_POS_X, SPAD_BALL_POS_Z);
    vx = func_8001E694(angle, (int32_t)speed);
    *(int16_t *)(player + 0x3CA) = vx;

    dy = (int32_t)SPAD_BALL_POS_Y - (int32_t)SPAD_TARGET_POS_Y;
    vy = (int16_t)(dy / frames);
    *(int16_t *)(player + 0x3CC) = vy;

    slot = player[0x392];
    team = player[0x398];
    attr_off = (team * 0x108) + (slot * 0x0C);
    stat_nibble = D_8010C32F[attr_off] & 0x0F;
    func_8005C454((int16_t *)(player + 0x3CC), 0x0C, -4 - ((int32_t)stat_nibble >> 1));

    vz = func_8001E5A4(angle, (int32_t)speed);
    *(int16_t *)(player + 0x3CE) = vz;

    /* Advance Scratchpad target coordinates by 6 frames of velocity */
    SPAD_TARGET_POS_X = (int16_t)((uint16_t)SPAD_TARGET_POS_X + (*(int16_t *)(player + 0x3CA) * frames));
    SPAD_TARGET_POS_Y = (int16_t)((uint16_t)SPAD_TARGET_POS_Y + (*(int16_t *)(player + 0x3CC) * frames));
    SPAD_TARGET_POS_Z = (int16_t)((uint16_t)SPAD_TARGET_POS_Z + (*(int16_t *)(player + 0x3CE) * frames));

    if (func_8003CEE4(player, 8, 1, 1, 0x14400) == 0) {
        return 0;
    }

    func_8005BA30(player, 0x2C, 0);
    player[0x3D1] = 0;

    slot = player[0x392];
    team = player[0x398];
    attr_off = (team * 0x108) + (slot * 0x0C);
    stat_flag = (*(uint32_t *)((uint8_t *)D_8010C330 + attr_off) >> 4) & 1;
    player[0x3AC] = (uint8_t)stat_flag;

    new_dir = func_8005B7CC(dir_u8, player[0x3AA], 0x28, stat_flag);
    player[0x3AA] = new_dir;

    facing_diff = 0x1C0 - (int32_t)new_dir;
    *(int16_t *)(player + 0x26) = (int16_t)((facing_diff % 256) << 4);
    player[0x3AB] = (uint8_t)dir_raw;

    return 1;
}
