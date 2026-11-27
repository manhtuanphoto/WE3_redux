/*
 * func_8018957C (enter overlay)
 *
 * Purpose:
 *   Pre-match stadium intro / entrance camera and phase state machine
 *   (ENTER.BIN), dispatched by Scratchpad+0x29B (sub-phase).
 *
 * Notes on function entry:
 *   In the split assembly (ENTER.yaml / spimdisasm), the two instructions
 *   immediately preceding 0x8018957C (at 0x80189574..0x80189578, labeled
 *   D_80189574 in 0.rodata.s: `lui $v1, 0x1F80; lbu $v1, 0x29B($v1)`) load
 *   Scratchpad+0x29B into $v1 before entering the stack frame setup at
 *   0x8018957C.
 *
 * Arguments:
 *   None (void; reads sub-phase from Scratchpad+0x29B)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read / written:
 *   D_800E6B84 (s16)          - checked == 0 before advancing sub-phase 1
 *   D_800E6B86 (u16)          - checked == 0 before advancing sub-phase 4 on timeout
 *   D_800E7804 (u8)           - set to 1 on exit in sub-phase 0xA when mode == 3
 *   D_800FF748 (u8[])         - actor table base (24 entries, stride 0x3E8) + match state:
 *     +0xAC4A (u8)            - cleared in sub-phase 0
 *     +0xAC4C (s16)           - cleared in sub-phase 0
 *     +0xABDA (u8)            - cleared in sub-phase 0x20
 *   D_8010C2A2 (u8)           - menu/screen transition state (set to 0x10 or 0xE)
 *   D_8018FDDC (5 x s16/u16[6], stride 10 bytes) - stadium orbital camera presets
 *   D_801918F4 (u8)           - set to 0x28 in sub-phase 0
 *   D_801918F5 (u8)           - random camera mode flag (0 or 1) from func_8001E6B8(2)
 *
 * Scratchpad (0x1F800000) fields read / written:
 *   +0x198 (s32)              - set to 4 in sub-phase 0
 *   +0x1E8 (u8), +0x1EC (u8)  - cleared to 0 in sub-phase 0
 *   +0x1F0..+0x204 (s32[6])   - camera eye/target coordinates
 *   +0x294 (u16)              - sub-phase frame timer
 *   +0x298 (u8)               - top-level game state (3 = menu/preview, etc.)
 *   +0x29B (u8)               - entrance sub-phase (0..4, 0xA, 0x20, 0x21)
 *   +0x2CE (u8)               - cleared to 0 in sub-phase 0
 *   +0x2E2 (u8)               - checked == 4 in sub-phase 2
 *   +0x2E3 (u8)               - stadium ID (0..5)
 *   +0x2E7 (u8)               - orbital interpolation divisor (4, or 6 for stadium 3)
 *   +0x2E8 (u16)              - orbital camera angle counter
 *   +0x2EA (u16)              - orbital camera radius
 *   +0x2F8 (s32)              - set to 0xFF in sub-phase 2
 *   +0x32C (s32)              - camera FOV / zoom parameter passed to func_800B53D8
 *   +0x331 (u8)               - cleared to 0 in sub-phase 2
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern signed short D_800E6B84;
extern unsigned short D_800E6B86;
extern unsigned char D_800E7804;
extern unsigned char D_800FF748[];
extern unsigned char D_8010C2A2;
extern unsigned char D_8018FDDC[];
extern unsigned char D_801918F4;
extern unsigned char D_801918F5;

extern void func_80017178(int state);
extern void func_800171DC(void);
extern void func_800171FC(int sub_phase);
extern void func_8001720C(void);
extern int func_8001E5A4(unsigned int angle, int scale);
extern int func_8001E694(unsigned int angle, int scale);
extern unsigned int func_8001E6B8(unsigned int range);
extern void func_8001F53C(void);
extern void func_8004FAC0(int a0, int a1);
extern unsigned int func_8004FB68(int a0);
extern unsigned int func_8004FBC4(int a0);
extern void func_8004FC54(int a0);
extern void func_80050888(unsigned int preset_group);
extern unsigned int func_800525A4(void);
extern void func_80055974(void);
extern void func_8005ADE4(void);
extern unsigned int func_80070008(void);
extern void func_800AB620(int cmd);
extern void func_800B53D8(int zoom);
extern unsigned int func_8018A7C4(void);
extern void func_8018F380(void);

void func_8018957C(void)
{
    unsigned int sub_phase = B(Scratchpad, 0x29B);
    unsigned int i;
    unsigned int stadium;
    unsigned int timer;

    if (sub_phase >= 0x22U) {
        return;
    }

    switch (sub_phase) {
    case 0:
        D_801918F4 = 0x28;
        B(Scratchpad, 0x1EC) = 0;
        B(Scratchpad, 0x1E8) = 0;
        SW(Scratchpad, 0x198) = 4;
        B(Scratchpad, 0x2CE) = 0;
        S(D_800FF748, 0xAC4C) = 0;
        B(D_800FF748, 0xAC4A) = 0;

        func_800AB620(0x51A);
        func_8001F53C();

        for (i = 0; i < 24U; i++) {
            B(D_800FF748, i * 0x3E8U) = 0;
        }

        stadium = B(Scratchpad, 0x2E3);
        if (stadium < 6U) {
            switch (stadium) {
            case 0:
                SW(Scratchpad, 0x1F0) = 0x7D80;
                SW(Scratchpad, 0x1F4) = -0x1600;
                SW(Scratchpad, 0x1FC) = -0x2800;
                SW(Scratchpad, 0x200) = -0x600;
                SW(Scratchpad, 0x1F8) = 0;
                SW(Scratchpad, 0x204) = 0;
                SW(Scratchpad, 0x32C) = 0x2C0;
                break;
            case 1:
                SW(Scratchpad, 0x1F0) = 0x7F00;
                SW(Scratchpad, 0x1F4) = -0x1800;
                SW(Scratchpad, 0x1FC) = -0x2C00;
                SW(Scratchpad, 0x200) = -0x400;
                SW(Scratchpad, 0x1F8) = 0;
                SW(Scratchpad, 0x204) = 0;
                SW(Scratchpad, 0x32C) = 0x200;
                break;
            case 2:
                SW(Scratchpad, 0x1F0) = 0x6400;
                SW(Scratchpad, 0x1F4) = -0x840;
                SW(Scratchpad, 0x1FC) = 0x2C00;
                SW(Scratchpad, 0x200) = -0xA00;
                SW(Scratchpad, 0x1F8) = 0;
                SW(Scratchpad, 0x204) = 0;
                SW(Scratchpad, 0x32C) = 0x200;
                break;
            case 3:
                SW(Scratchpad, 0x1F0) = 0x4510;
                SW(Scratchpad, 0x1F4) = -0x7F0;
                SW(Scratchpad, 0x1FC) = -0x5510;
                SW(Scratchpad, 0x200) = 0x12F0;
                SW(Scratchpad, 0x1F8) = 0;
                SW(Scratchpad, 0x204) = 0;
                SW(Scratchpad, 0x32C) = 0xD3;
                break;
            case 4:
                SW(Scratchpad, 0x1F0) = 0x289E;
                SW(Scratchpad, 0x1F4) = -0x1C0;
                SW(Scratchpad, 0x1FC) = -0x4FA7;
                SW(Scratchpad, 0x200) = -0xB70;
                SW(Scratchpad, 0x1F8) = 0;
                SW(Scratchpad, 0x204) = 0;
                SW(Scratchpad, 0x32C) = 0x150;
                break;
            case 5:
                SW(Scratchpad, 0x1F0) = 0x1DAF;
                SW(Scratchpad, 0x1F4) = -0xE90;
                SW(Scratchpad, 0x1FC) = -0x4FA7;
                SW(Scratchpad, 0x200) = 0x2E0;
                SW(Scratchpad, 0x1F8) = 0;
                SW(Scratchpad, 0x204) = 0;
                SW(Scratchpad, 0x32C) = 0xC7;
                break;
            }
        }

        func_800B53D8(SW(Scratchpad, 0x32C));
        func_80050888(1U);
        H(Scratchpad, 0x294) = 0;

        if (B(Scratchpad, 0x298) == 3) {
            func_800171FC(0x20);
        } else {
            func_800171DC();
        }
        break;

    case 1:
        timer = H(Scratchpad, 0x294);
        H(Scratchpad, 0x294) = (unsigned short)(timer + 1U);
        if (timer < 5U) {
            break;
        }
        if (func_8004FB68(0) == 0) {
            break;
        }
        if (D_800E6B84 != 0) {
            break;
        }
        func_800171DC();
        break;

    case 2:
        if (B(Scratchpad, 0x2E2) != 4) {
            unsigned int action = func_80070008();
            if (action == 1U) {
                func_8004FAC0(0x10, 0);
                func_80017178(3);
                D_8010C2A2 = 0x10;
                break;
            }
            if (action != 2U) {
                break;
            }
        }

        func_8005ADE4();
        func_8018F380();
        SW(Scratchpad, 0x2F8) = 0xFF;
        func_80055974();
        B(Scratchpad, 0x331) = 0;
        H(Scratchpad, 0x294) = 0;
        func_800171DC();
        break;

    case 3:
        timer = H(Scratchpad, 0x294);
        H(Scratchpad, 0x294) = (unsigned short)(timer + 1U);
        if (timer >= 0x78U && func_8004FBC4(0) != 0) {
            D_801918F5 = (unsigned char)func_8001E6B8(2U);
            if (D_801918F5 != 0) {
                unsigned int off = B(Scratchpad, 0x2E3) * 10U;

                SW(Scratchpad, 0x1F4) = S(D_8018FDDC, off + 0U);
                H(Scratchpad, 0x2EA) = H(D_8018FDDC, off + 2U);
                SW(Scratchpad, 0x200) = S(D_8018FDDC, off + 4U);
                SW(Scratchpad, 0x32C) = S(D_8018FDDC, off + 6U);
                H(Scratchpad, 0x2E8) = H(D_8018FDDC, off + 8U);

                B(Scratchpad, 0x2E7) = (B(Scratchpad, 0x2E3) == 3) ? 6 : 4;
                SW(Scratchpad, 0x1F0) = 0;
                SW(Scratchpad, 0x1F8) = 0;
                func_800B53D8(SW(Scratchpad, 0x32C));
                func_80050888(3U);
            } else {
                stadium = B(Scratchpad, 0x2E3);
                if (stadium < 6U) {
                    switch (stadium) {
                    case 0:
                        SW(Scratchpad, 0x1F8) = -0x4800;
                        SW(Scratchpad, 0x1F0) = -0x5000;
                        SW(Scratchpad, 0x1F4) = -0x100;
                        SW(Scratchpad, 0x204) = 0x1800;
                        SW(Scratchpad, 0x1FC) = 0x2000;
                        SW(Scratchpad, 0x200) = -0x1000;
                        SW(Scratchpad, 0x32C) = 0x400;
                        break;
                    case 1:
                    case 2:
                        SW(Scratchpad, 0x1F8) = -0x4800;
                        SW(Scratchpad, 0x1F0) = -0x5000;
                        SW(Scratchpad, 0x1F4) = -0x100;
                        SW(Scratchpad, 0x1FC) = 0x2000;
                        SW(Scratchpad, 0x204) = 0x400;
                        SW(Scratchpad, 0x200) = -0x1000;
                        SW(Scratchpad, 0x32C) = 0x400;
                        break;
                    case 3:
                        SW(Scratchpad, 0x1F0) = 0x4970;
                        SW(Scratchpad, 0x1F4) = -0x1094;
                        SW(Scratchpad, 0x1F8) = 0x1C29;
                        SW(Scratchpad, 0x1FC) = -0x44B0;
                        SW(Scratchpad, 0x200) = -0x32;
                        SW(Scratchpad, 0x204) = -0x3F20;
                        SW(Scratchpad, 0x32C) = 0x112;
                        break;
                    case 4:
                        SW(Scratchpad, 0x1F0) = -0x8A9;
                        SW(Scratchpad, 0x1F4) = -0x150;
                        SW(Scratchpad, 0x1F8) = -0x31DE;
                        SW(Scratchpad, 0x1FC) = -0x6040;
                        SW(Scratchpad, 0x200) = -0xEC0;
                        SW(Scratchpad, 0x204) = 0x1311;
                        SW(Scratchpad, 0x32C) = 0x2E0;
                        break;
                    case 5:
                        SW(Scratchpad, 0x1F0) = -0x1019;
                        SW(Scratchpad, 0x1F4) = -0xD70;
                        SW(Scratchpad, 0x1F8) = -0x31DE;
                        SW(Scratchpad, 0x1FC) = -0x5A80;
                        SW(Scratchpad, 0x200) = -0x14E0;
                        SW(Scratchpad, 0x204) = 0x2D91;
                        SW(Scratchpad, 0x32C) = 0x127;
                        break;
                    }
                }
                func_80050888(2U);
                func_800B53D8(SW(Scratchpad, 0x32C));
            }
            func_800171DC();
            H(Scratchpad, 0x294) = 0;
        }

        if (func_800525A4() != 0) {
            B(Scratchpad, 0x29B) = 0xA;
        }
        break;

    case 4:
        timer = H(Scratchpad, 0x294);
        if (timer < 5U) {
            H(Scratchpad, 0x294) = (unsigned short)(timer + 1U);
            break;
        }

        if (D_801918F5 != 0) {
            int angle_raw = S(Scratchpad, 0x2E8);
            int step = (int)B(Scratchpad, 0x2E7);
            int q = angle_raw / step;
            int rem = angle_raw % step;
            int w0 = step - rem;
            int radius = S(Scratchpad, 0x2EA);
            unsigned int idx0 = (unsigned int)q & 0xFFU;
            unsigned int idx1 = (unsigned int)(q + 1) & 0xFFU;
            int c0 = func_8001E694(idx0, radius);
            int c1 = func_8001E694(idx1, radius);
            int s0;
            int s1;

            SW(Scratchpad, 0x1FC) = (c0 * w0 + c1 * rem) / step;

            s0 = func_8001E5A4(idx0, radius);
            s1 = func_8001E5A4(idx1, radius);
            SW(Scratchpad, 0x204) = (s0 * w0 + s1 * rem) / step;

            H(Scratchpad, 0x2E8) = (unsigned short)(H(Scratchpad, 0x2E8) + 1U);
        }

        if (func_8004FB68(0) == 0) {
            break;
        }

        timer = H(Scratchpad, 0x294);
        H(Scratchpad, 0x294) = (unsigned short)(timer + 1U);
        if (timer >= 0xF1U && D_800E6B86 == 0) {
            func_800171DC();
            B(Scratchpad, 0x29B) = 0x20;
        }

        if (func_800525A4() != 0) {
            B(Scratchpad, 0x29B) = 0x20;
        }
        break;

    case 0xA:
        if (B(Scratchpad, 0x298) == 3) {
            D_800E7804 = 1;
            B(Scratchpad, 0x298) = 3;
            D_8010C2A2 = 0xE;
        } else if (func_8004FBC4(0) != 0) {
            SW(Scratchpad, 0x32C) = 0x400;
            func_800B53D8(0x400);
            func_8004FC54(1);
            func_8001720C();
        }
        break;

    case 0x20:
        B(D_800FF748, 0xABDA) = 0;
        B(Scratchpad, 0x29B) = (unsigned char)(B(Scratchpad, 0x29B) + 1U);
        break;

    case 0x21:
        if (func_8018A7C4() != 0) {
            B(Scratchpad, 0x29B) = 0xA;
        }
        break;

    default:
        break;
    }
}
