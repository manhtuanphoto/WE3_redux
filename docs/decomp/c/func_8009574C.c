/*
 * func_8009574C
 *
 * Purpose:
 *   Initializes the goalkeeper penalty-kick / free-kick dive or set-piece
 *   reaction state for the goalkeeper actor identified by scratch 0x2AE,
 *   choosing dive direction/distance/height from CPU random roll or human
 *   pad input (+0x3D4) and D_800F0EA1 (kick type/zone).
 *
 * Arguments:
 *   None (reads target actor ID from scratch 0x1F8002AE).
 *
 * Return value:
 *   void.
 *
 * Scratchpad (0x1F800000) reads/writes:
 *   - 0x0F2 (s16, W): target height y for high-corner dive (0x8E).
 *   - 0x0FC (s16, R/W): target z computed by func_800484F8, jittered and clamped to +-0x280.
 *   - 0x238/0x23C (u16, R): pad button masks when 0x2E2 == 2.
 *   - 0x29A (u8, R): match phase (8 = penalty shootout / PK mode).
 *   - 0x2AE (u8, R): target goalkeeper actor ID.
 *   - 0x2E2 (u8, R): control/replay mode (2 = use 0x238/0x23C directly).
 *   - 0x2F4 (unsigned char *, R): pointer to ball actor.
 *   - 0x31C (s16, W): ball/reaction timer set from GK stat (+0x3B9 / 10).
 *
 * Actor fields reads/writes (actor = D_800FF748 + (scratch[0x2AE] % 24) * 1000):
 *   - +0x02/+0x06 (s16, R): position x/z.
 *   - +0x392 (u8, R): team slot (0..10).
 *   - +0x396 (u8, W): state (0x89, 0x8E, 0x8F, 0x93, 0x96, or 0x9B).
 *   - +0x397 (u8, W): substate (0, or 0x10 for states 0x89/0x9B).
 *   - +0x398 (u8, R): side (0/1).
 *   - +0x399 (u8, R): attack direction flag (0/1).
 *   - +0x39A (u8, R): CPU control flag.
 *   - +0x3A0 (u32, W): set to 0x30 (48 raw/frame = 0.192 m/frame = 5.76 m/s).
 *   - +0x3AB (u8, W): target facing heading (0/128, or ball_heading + 128).
 *   - +0x3AC (u8, W): mirror/dive-side flag (0 or 1).
 *   - +0x3B4 (u8, R): max speed stat byte.
 *   - +0x3B9 (u8, R): GK response stat (divided by 10 to adjust 0x31C timer).
 *   - +0x3C2 (u16, W): latched x position.
 *   - +0x3C4 (s16, W): dive height/power parameter (-0x20 or -0x200).
 *   - +0x3C6 (s16, W): dive lateral target z (+-384, +-512, +-640, or clamped 0xFC).
 *   - +0x3D0 (u8, W): set to 0xFF.
 *   - +0x3D4/+0x3D6 (u16, R/W): input/dive direction button mask (bits 0xE000).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x280 (640): +-2.56 m dive lateral Z clamp (goalpost half-width ~3.66 m).
 *   - -0x200 (-512): -2.048 m high dive Y parameter; -0x20 (-32): -0.128 m low dive.
 *   - 640 / 512 / 384 (2.56 m / 2.048 m / 1.536 m): lateral dive Z offsets.
 *   - 0x28 (40): +-0.16 m Z jitter on high dive.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];
extern unsigned char D_800F0EA1;
extern unsigned int D_8010C330[];

extern int func_8001E6B8(int n);
extern void func_8005C7EC(unsigned char *actor);
extern int func_800484F8(int x, int z, int speed_arg, int mode);
extern int func_8005C7A0(int n);

void func_8009574C(void)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *actor = D_800FF748 + (B(scratch, 0x2AE) % 24) * 1000;
    int side_sign;
    unsigned int btn_dir;
    unsigned char kick_type;

    if (B(scratch, 0x29A) == 8) {
        B(actor, 0x3AB) = 0;
        side_sign = 1;
    } else {
        B(actor, 0x3AB) = B(actor, 0x399) << 7;
        side_sign = (B(actor, 0x399) != 0) ? -1 : 1;
    }

    W(actor, 0x3A0) = 0x30;

    if (B(actor, 0x39A) != 0) {
        switch (func_8001E6B8(9)) {
        case 0:
        case 1:
            H(actor, 0x3D4) = 0xC000;
            break;
        case 2:
        case 3:
            H(actor, 0x3D4) = 0x6000;
            break;
        case 4:
        case 5:
            H(actor, 0x3D4) = 0x8000;
            break;
        case 6:
        case 7:
            H(actor, 0x3D4) = 0x2000;
            break;
        default:
            H(actor, 0x3D4) = 0;
            break;
        }
    } else if (B(scratch, 0x2E2) == 2) {
        H(actor, 0x3D6) = H(scratch, 0x23C);
        H(actor, 0x3D4) = H(scratch, 0x238);
    } else {
        func_8005C7EC(actor);
    }

    B(actor, 0x3D0) = 0xFF;
    H(actor, 0x3C2) = H(actor, 2);
    btn_dir = H(actor, 0x3D4) & 0xE000;

    if (btn_dir == 0xC000) {
        B(actor, 0x3AC) = 0;
        kick_type = D_800F0EA1;
        if (kick_type == 0 || kick_type == 3) {
            S(actor, 0x3C4) = -0x20;
            S(actor, 0x3C6) = (-side_sign) * 640;
            S(scratch, 0x31C) = 0xE - (B(actor, 0x3B9) / 10) - B(actor, 0x399);
            B(actor, 0x396) = 0x8F;
            B(actor, 0x397) = 0;
        } else {
            S(scratch, 0x31C) = 0x12 - (B(actor, 0x3B9) / 10) - B(actor, 0x399);
            S(actor, 0x3C6) = (-side_sign) * 384;
            B(actor, 0x396) = 0x93;
            B(actor, 0x397) = 0;
        }
    } else if (btn_dir == 0x6000) {
        B(actor, 0x3AC) = 1;
        kick_type = D_800F0EA1;
        if (kick_type == 2 || kick_type == 5) {
            S(actor, 0x3C4) = -0x20;
            S(actor, 0x3C6) = side_sign * 640;
            S(scratch, 0x31C) = 0xE - (B(actor, 0x3B9) / 10) - B(actor, 0x399);
            B(actor, 0x396) = 0x8F;
            B(actor, 0x397) = 0;
        } else {
            S(scratch, 0x31C) = 0x12 - (B(actor, 0x3B9) / 10) - B(actor, 0x399);
            S(actor, 0x3C6) = side_sign * 384;
            B(actor, 0x396) = 0x93;
            B(actor, 0x397) = 0;
        }
    } else if (btn_dir == 0x8000) {
        B(actor, 0x3AC) = 0;
        kick_type = D_800F0EA1;
        if (kick_type == 0) {
            unsigned int stat_word;
            int z;
            S(actor, 0x3C4) = -0x200;
            stat_word = *(unsigned int *)((unsigned char *)D_8010C330 +
                                          B(actor, 0x392) * 12 + B(actor, 0x398) * 264);
            S(scratch, 0xF2) = -((int)((((stat_word >> 11) & 0x7F) + 100) * 352)) / 180;
            func_800484F8(S(actor, 2), S(actor, 6), (B(actor, 0x3B4) - 1) & 0xFFFF, 2);
            z = S(scratch, 0xFC) + func_8005C7A0(0x28);
            if (z >= 0x281) {
                z = 0x280;
            } else if (z < -0x280) {
                z = -0x280;
            }
            S(scratch, 0xFC) = z;
            S(actor, 0x3C6) = S(scratch, 0xFC);
            S(scratch, 0x31C) = 0x12 - (B(actor, 0x3B9) / 10);
            B(actor, 0x396) = 0x8E;
            B(actor, 0x397) = 0;
        } else if (kick_type == 3) {
            S(actor, 0x3C4) = -0x20;
            S(actor, 0x3C6) = (-side_sign) << 9;
            S(scratch, 0x31C) = 0xD - (B(actor, 0x3B9) / 10);
            B(actor, 0x396) = 0x8F;
            B(actor, 0x397) = 0;
        } else {
            S(actor, 0x3C6) = (-side_sign) * 384;
            S(scratch, 0x31C) = 0x12 - (B(actor, 0x3B9) / 10);
            B(actor, 0x396) = 0x93;
            B(actor, 0x397) = 0;
        }
    } else if (btn_dir == 0x2000) {
        B(actor, 0x3AC) = 1;
        kick_type = D_800F0EA1;
        if (kick_type == 2) {
            unsigned int stat_word;
            int z;
            S(actor, 0x3C4) = -0x200;
            stat_word = *(unsigned int *)((unsigned char *)D_8010C330 +
                                          B(actor, 0x392) * 12 + B(actor, 0x398) * 264);
            S(scratch, 0xF2) = -((int)((((stat_word >> 11) & 0x7F) + 100) * 352)) / 180;
            func_800484F8(S(actor, 2), S(actor, 6), (B(actor, 0x3B4) - 1) & 0xFFFF, 2);
            z = S(scratch, 0xFC) + func_8005C7A0(0x28);
            if (z >= 0x281) {
                z = 0x280;
            } else if (z < -0x280) {
                z = -0x280;
            }
            S(scratch, 0xFC) = z;
            S(actor, 0x3C6) = S(scratch, 0xFC);
            S(scratch, 0x31C) = 0x12 - (B(actor, 0x3B9) / 10);
            B(actor, 0x396) = 0x8E;
            B(actor, 0x397) = 0;
        } else if (kick_type == 5) {
            S(actor, 0x3C4) = -0x20;
            S(actor, 0x3C6) = side_sign << 9;
            S(scratch, 0x31C) = 0xD - (B(actor, 0x3B9) / 10);
            B(actor, 0x396) = 0x8F;
            B(actor, 0x397) = 0;
        } else {
            S(actor, 0x3C6) = side_sign * 384;
            S(scratch, 0x31C) = 0x12 - (B(actor, 0x3B9) / 10);
            B(actor, 0x396) = 0x93;
            B(actor, 0x397) = 0;
        }
    } else {
        unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
        kick_type = D_800F0EA1;
        if (kick_type == 1) {
            B(actor, 0x396) = 0x9B;
            B(actor, 0x397) = 0x10;
            B(actor, 0x3AB) = B(ball, 0x3A4) + 0x80;
        } else if (kick_type == 4) {
            B(actor, 0x396) = 0x89;
            B(actor, 0x397) = 0x10;
            B(actor, 0x3AB) = B(ball, 0x3A4) + 0x80;
        } else {
            S(scratch, 0x31C) = B(actor, 0x399);
            B(actor, 0x396) = 0x96;
            B(actor, 0x397) = 0;
        }
    }
}
