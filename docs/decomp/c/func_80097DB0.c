/*
 * func_80097DB0
 * Purpose: Updates the replay/goal-celebration camera eye (0x1F8001F0..0x1F8) and
 *          look-at target (0x1F8001FC..0x204) positions based on camera sub-mode
 *          scratchpad 0x2F2 (cases 0..13).
 * Arguments: none (void)
 * Return value: void
 * Actor fields read:
 *   - +0x02 (s16): X position (250 raw = 1 m)
 *   - +0x04 (s16): Y position / height (negative = up)
 *   - +0x06 (s16): Z position (250 raw = 1 m)
 *   - +0x399 (u8): attack direction flag (0/1)
 * Scratchpad fields read/written:
 *   - 0x1F8001F0/1F4/1F8 (s32): camera eye X/Y/Z
 *   - 0x1F8001FC/200/204 (s32): camera target X/Y/Z
 *   - 0x1F8002AD (u8): actor index used in case 3 (mod 24)
 *   - 0x1F8002AE (u8): focus actor index (mod 24)
 *   - 0x1F8002E3 (u8): camera variant selector in case 3 (1 or 5 vs other)
 *   - 0x1F8002E7 (u8): camera orbit angle byte (0..255 = 360 deg)
 *   - 0x1F8002EA (s16): camera orbit distance
 *   - 0x1F8002F2 (u8): camera sub-mode (0..13; incremented or set to 0xFF)
 *   - 0x1F8002F4 (ptr): pointer to ball actor
 *   - 0x1F800314 (s32[2]): per-side goal-line/anchor X coordinate
 * External data read:
 *   - D_800FF748: base of 24-element, 1000-byte actor array
 * Magic numbers:
 *   - -0x200 (-512 raw = -2.048 m): standard player torso camera target height
 *   - 0x23E2 (9186 raw = 36.744 m): minimum abs(X) clamp for ball-in-box camera
 *   - 0x1FE2 (8162 raw = 32.648 m): camera eye X position in case 4
 *   - 0x3520 (13600 raw = 54.4 m): out-of-bounds X threshold setting 0x2F2 = 0xFF
 *   - 0x8B6 (2230 raw = 8.92 m) / 0x400 (1024 raw = 4.096 m): Z clamp thresholds
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCHPAD ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];

extern int func_8001E694(unsigned int, int);
extern int func_8001E5A4(unsigned int, int);

void func_80097DB0(void)
{
    unsigned char *scratch = SCRATCHPAD;
    unsigned char actorIdx = B(scratch, 0x2AE) % 24U;
    unsigned char *actor = D_800FF748 + actorIdx * 1000;
    unsigned char mode = B(scratch, 0x2F2);

    if (mode >= 0xE) {
        return;
    }

    switch (mode) {
    case 0: {
        int offX, offZ;
        SW(scratch, 0x200) = -0x200;
        SW(scratch, 0x1FC) = S(actor, 0x2);
        SW(scratch, 0x204) = S(actor, 0x6);
        offX = func_8001E694(B(scratch, 0x2E7), S(scratch, 0x2EA));
        SW(scratch, 0x1F0) = SW(scratch, 0x1FC) + offX;
        SW(scratch, 0x1F4) = (-S(scratch, 0x2EA)) / 4;
        offZ = func_8001E5A4(B(scratch, 0x2E7), S(scratch, 0x2EA));
        SW(scratch, 0x1F8) = SW(scratch, 0x204) + offZ;
        break;
    }
    case 1: {
        int y, offX, offZ;
        SW(scratch, 0x1FC) = S(actor, 0x2);
        y = S(actor, 0x4);
        SW(scratch, 0x200) = y;
        if (y >= -0x1FF) {
            SW(scratch, 0x200) = -0x200;
        }
        SW(scratch, 0x204) = S(actor, 0x6);
        offX = func_8001E694(B(scratch, 0x2E7), S(scratch, 0x2EA));
        SW(scratch, 0x1F4) = -0xA00;
        SW(scratch, 0x1F0) = SW(scratch, 0x1FC) + offX;
        offZ = func_8001E5A4(B(scratch, 0x2E7), S(scratch, 0x2EA));
        SW(scratch, 0x1F8) = SW(scratch, 0x204) + offZ;
        break;
    }
    case 2: {
        int halfDist = (S(scratch, 0x2EA) - 0x1C00) >> 1;
        int offX = func_8001E694(B(scratch, 0x2E7), halfDist);
        int offZ;
        SW(scratch, 0x200) = -0x200;
        SW(scratch, 0x1FC) = S(actor, 0x2) + offX;
        offZ = func_8001E5A4(B(scratch, 0x2E7), halfDist);
        SW(scratch, 0x204) = S(actor, 0x6) + offZ;
        offX = func_8001E694(B(scratch, 0x2E7), 0x2800);
        SW(scratch, 0x1F4) = -0x4000;
        SW(scratch, 0x1F0) = SW(scratch, 0x1FC) + offX;
        offZ = func_8001E5A4(B(scratch, 0x2E7), 0x2800);
        SW(scratch, 0x1F8) = SW(scratch, 0x204) + offZ;
        break;
    }
    case 3: {
        unsigned char idx3 = B(scratch, 0x2AD) % 24U;
        unsigned char *actor3 = D_800FF748 + idx3 * 1000;
        int targetX = SW(scratch, 0x314 + (B(actor3, 0x399) ^ 1) * 4);
        unsigned char camVar = B(scratch, 0x2E3);

        SW(scratch, 0x200) = -0x200;
        SW(scratch, 0x204) = 0x1080;
        SW(scratch, 0x1FC) = targetX;
        SW(scratch, 0x1F0) = SW(scratch, 0x314 + (B(actor3, 0x399) ^ 1) * 4);
        if (camVar == 1 || camVar == 5) {
            SW(scratch, 0x1F4) = -0x120;
            SW(scratch, 0x1F8) = -0x3200;
        } else {
            SW(scratch, 0x1F4) = -0x600;
            SW(scratch, 0x1F8) = -0x4400;
        }
        break;
    }
    case 4: {
        unsigned char *ball = *(unsigned char **)(scratch + 0x2F4);
        int bx = S(ball, 0x2);
        int absX = bx >= 0 ? bx : -bx;

        SW(scratch, 0x1FC) = bx;
        SW(scratch, 0x200) = S(ball, 0x4);
        SW(scratch, 0x204) = S(ball, 0x6);
        if (absX < 0x23E2) {
            SW(scratch, 0x1FC) = (S(ball, 0x2) >= 0) ? 0x23E2 : -0x23E2;
        }
        SW(scratch, 0x1F0) = (S(ball, 0x2) >= 0) ? 0x1FE2 : -0x1FE2;
        SW(scratch, 0x1F4) = -0x1000;
        SW(scratch, 0x1F8) = (S(ball, 0x6) > 0) ? -0x2500 : 0x2500;
        B(scratch, 0x2F2)++;
        break;
    }
    case 5: {
        unsigned char *ball = *(unsigned char **)(scratch + 0x2F4);
        int bx = S(ball, 0x2);
        int absX = bx >= 0 ? bx : -bx;
        int checkX;

        if (absX < 0x23E2) {
            int clampX = (bx >= 0) ? 0x23E2 : -0x23E2;
            SW(scratch, 0x1FC) = (SW(scratch, 0x1FC) * 9 + clampX) / 10;
        } else {
            SW(scratch, 0x1FC) = (SW(scratch, 0x1FC) * 9 + bx) / 10;
        }
        SW(scratch, 0x200) = (SW(scratch, 0x200) * 9 + S(ball, 0x4)) / 10;
        SW(scratch, 0x204) = (SW(scratch, 0x204) * 9 + S(ball, 0x6)) / 10;

        checkX = S(ball, 0x2);
        if ((checkX >= 0 ? checkX : -checkX) >= 0x3520) {
            B(scratch, 0x2F2) = 0xFF;
        }
        break;
    }
    case 6:
    case 7:
    case 10:
    case 11:
        break;
    case 8: {
        unsigned char *ball = *(unsigned char **)(scratch + 0x2F4);
        SW(scratch, 0x1FC) = (S(ball, 0x2) >= 0) ? 0x23E2 : -0x23E2;
        SW(scratch, 0x200) = 0;
        SW(scratch, 0x204) = 0;
        SW(scratch, 0x1F0) = (S(ball, 0x2) >= 0) ? 0x6120 : -0x6120;
        SW(scratch, 0x1F4) = -0xE00;
        SW(scratch, 0x1F8) = 0;
        B(scratch, 0x2F2)++;
        break;
    }
    case 9: {
        unsigned char *ball = *(unsigned char **)(scratch + 0x2F4);
        int bz = S(ball, 0x6);
        int checkX;

        if (bz >= 0x8B7) {
            SW(scratch, 0x204) = (SW(scratch, 0x204) * 9 + 0x8B6) / 10;
        } else if (bz < -0x8B6) {
            SW(scratch, 0x204) = (SW(scratch, 0x204) * 9 - 0x8B6) / 10;
        } else if ((bz >= 0 ? bz : -bz) < 0x400) {
            SW(scratch, 0x204) = (SW(scratch, 0x204) * 9) / 10;
        }

        checkX = S(ball, 0x2);
        if ((checkX >= 0 ? checkX : -checkX) >= 0x3520) {
            B(scratch, 0x2F2) = 0xFF;
        }
        break;
    }
    case 12:
        SW(scratch, 0x200) = -0x140;
        SW(scratch, 0x1FC) = S(actor, 0x2);
        SW(scratch, 0x204) = S(actor, 0x6);
        SW(scratch, 0x1F4) = -0x800;
        SW(scratch, 0x1F0) = S(actor, 0x2);
        SW(scratch, 0x1F8) = S(actor, 0x6) - 0x2000;
        B(scratch, 0x2F2)++;
        break;
    case 13:
        SW(scratch, 0x1FC) = (SW(scratch, 0x1FC) * 9 + S(actor, 0x2)) / 10;
        SW(scratch, 0x204) = (SW(scratch, 0x204) * 9 + S(actor, 0x6)) / 10;
        break;
    }
}
