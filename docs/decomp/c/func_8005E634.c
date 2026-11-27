#include <stdint.h>

extern uint8_t D_80012348[8];
extern uint8_t D_80012350[8];
extern uint8_t D_80012358[8];
extern uint8_t D_80012360[8];
extern uint8_t D_80012368[4];
extern uint8_t D_8001236C[4];
extern uint8_t D_80012370[4];
extern uint8_t D_80012374[4];
extern uint8_t D_80012378[8];
extern uint8_t D_80012380[8];
extern uint8_t D_80012388[8];
extern uint8_t D_80012390[8];

extern uint8_t D_800C84A8[8];
extern uint8_t D_800C84B0[8];
extern uint8_t D_800C84D8[4];
extern uint8_t D_800C84DC[4];
extern uint8_t D_800C84E0[8];
extern uint8_t D_800C84E8[8];

/*
 * Copies preset byte tables from D_80012348..D_80012390 (via unaligned stack
 * copies in the original MIPS) into globals D_800C84A8..D_800C84E8 based on mode.
 * UNCLEAR: Semantic meaning of D_80012348..D_80012390 and D_800C84A8..D_800C84E8
 * (likely UI/display/controller/config preset tables).
 */
void func_8005E634(uint8_t mode) {
    uint8_t buf_00[8];
    uint8_t buf_08[8];
    uint8_t buf_10[8];
    uint8_t buf_18[8];
    uint8_t buf_20[4];
    uint8_t buf_28[4];
    uint8_t buf_30[4];
    uint8_t buf_38[4];
    uint8_t buf_40[8];
    uint8_t buf_48[8];
    uint8_t buf_50[8];
    uint8_t buf_58[8];
    uint32_t i;

    for (i = 0; i < 8; i++) {
        buf_00[i] = D_80012348[i];
        buf_08[i] = D_80012350[i];
        buf_10[i] = D_80012358[i];
        buf_18[i] = D_80012360[i];
        buf_40[i] = D_80012378[i];
        buf_48[i] = D_80012380[i];
        buf_50[i] = D_80012388[i];
        buf_58[i] = D_80012390[i];
    }
    for (i = 0; i < 4; i++) {
        buf_20[i] = D_80012368[i];
        buf_28[i] = D_8001236C[i];
        buf_30[i] = D_80012370[i];
        buf_38[i] = D_80012374[i];
    }

    switch (mode) {
        case 0:
            for (i = 0; i < 8; i++) {
                D_800C84A8[i] = buf_00[i];
                D_800C84B0[i] = buf_10[i];
                D_800C84E0[i] = buf_40[i];
                D_800C84E8[i] = buf_50[i];
            }
            break;

        case 1:
            for (i = 0; i < 8; i++) {
                D_800C84A8[i] = buf_08[i];
                D_800C84B0[i] = buf_18[i];
                D_800C84E0[i] = buf_48[i];
                D_800C84E8[i] = buf_58[i];
            }
            break;

        case 2:
            for (i = 0; i < 4; i++) {
                D_800C84D8[i] = buf_28[i];
                D_800C84DC[i] = buf_38[i];
            }
            break;

        case 3:
            for (i = 0; i < 4; i++) {
                D_800C84D8[i] = buf_20[i];
                D_800C84DC[i] = buf_30[i];
            }
            break;

        default:
            break;
    }
}
