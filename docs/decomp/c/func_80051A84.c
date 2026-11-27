/*
 * func_80051A84
 *
 * Purpose:
 *   Computes the 3D camera eye position (out+0x00..0x08) and look-at target
 *   position (out+0x0C..0x14) from the active camera preset (scratch 0x2E6),
 *   camera update mode (scratch 0x2F2), and ball/owner positions, applying
 *   exponential smoothing (4:1 EMA) in live modes or direct placement in snap mode.
 *
 * Arguments:
 *   out - pointer to 24-byte (6 x s32) camera vector struct:
 *         +0x00/+0x04/+0x08 = eye X/Y/Z, +0x0C/+0x10/+0x14 = target X/Y/Z
 *
 * Return value:
 *   void
 *
 * Scratchpad (0x1F800000) reads/writes:
 *   - 0x1EC (u8, W): camera style flag (1 for modes 0..4, 3 for modes 5..7)
 *   - 0x1F0/0x1F4/0x1F8 (s32, R): previous camera eye X/Y/Z for EMA smoothing
 *   - 0x1FC/0x200/0x204 (s32, R): previous camera target X/Y/Z for EMA smoothing
 *   - 0x28A (s16, R/W): camera horizontal X lead offset
 *   - 0x2A0 (u8, R): flag gating automatic 0x2EA Z-offset expansion
 *   - 0x2A9 (u8, R): ball owner actor id (0 = free)
 *   - 0x2AC (u8, R): actor id used for attack-direction lead in mode 0
 *   - 0x2AD (u8, R): actor id used for attack-direction lead in mode 3
 *   - 0x2B4/0x2B6 (s16, R): predicted ball X/Z target (used when 0x2F2 == 1)
 *   - 0x2E0 (u8, R): active side index (used for side-cam facing when 0x2E2 != 1,4)
 *   - 0x2E2 (u8, R): camera sub-mode override
 *   - 0x2E6 (u8, R): camera preset index (0..4 standard, 5..7 side/vertical, >=8 no-op)
 *   - 0x2E7 (u8, W): camera angle/orientation byte (0x40, 0x00, or 0x80)
 *   - 0x2EA (s16, R/W): dynamic camera Z pullback offset
 *   - 0x2EE (s16, R/W): camera snap/lock countdown timer in frames
 *   - 0x2F2 (u8, R): camera tracking mode (0 = follow owner/ball, 1 = blend ball+target, 3 = instant snap)
 *   - 0x2F4 (u32, R): pointer to ball actor
 *
 * Actor fields read:
 *   - +0x02 (s16): X position
 *   - +0x04 (s16): Y position (height, negative = up)
 *   - +0x06 (s16): Z position
 *   - +0x14 (s16): ball Z-related velocity/magnitude (checked >= 0x91)
 *   - +0x399 (u8): attack direction flag (0 or 1)
 *
 * Globals read:
 *   - D_800C7CA4: camera preset table (18 bytes per entry):
 *       +0x00 (s16): max X lead
 *       +0x02 (s16): target Y
 *       +0x04 (s16): side-camera eye X offset
 *       +0x06 (s16): eye Y
 *       +0x08 (s16): eye Z pullback distance
 *       +0x0A (s16): max target Z
 *       +0x0C (s16): min target Z
 *       +0x0E (s16): max abs target X
 *       +0x10 (u8):  eye X parallax divisor (eye_x = 100 * tx / div)
 *   - D_800FF748: 24 * 1000-byte actor pool
 *
 * Magic numbers / units (250 raw = 1 m, 30 frames/s):
 *   - 0x30 (48) = 0.192 m/frame (0.576 m/s? at 30 Hz = 5.76 m/s) X lead slew rate
 *   - 0x91 (145) = 0.58 m/frame threshold on ball +0x14 to trigger 0x2EA pullback
 *   - 0x60 (96) = 0.384 m increment applied to 0x2EA pullback
 *   - -0x4A00 (-18944) = -75.776 m Z clamp threshold (unsigned comparison in asm)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800C7CA4[];
extern unsigned char D_800FF748[];

void func_80051A84(unsigned char *out)
{
    unsigned char camMode;
    signed short timer;
    unsigned char *camCfg;
    signed short tx;
    signed short tz;
    int sideDir;

    camMode = B(Scratchpad, 0x2E6);
    timer = S(Scratchpad, 0x2EE);
    camCfg = D_800C7CA4 + (unsigned int)camMode * 18;

    if (timer != 0) {
        S(Scratchpad, 0x2EE) = timer - 1;
    }

    if (camMode < 5) {
        B(Scratchpad, 0x1EC) = 1;
        B(Scratchpad, 0x2E7) = 0x40;

        switch (B(Scratchpad, 0x2F2)) {
        case 0:
        {
            unsigned char *focusActor;
            unsigned char ownerId;
            unsigned char *ball;
            signed short lead;
            signed short maxLead;
            signed short maxX;
            signed short maxZ;
            signed short minZ;
            int scaledX;

            focusActor = D_800FF748 + (B(Scratchpad, 0x2AC) % 24) * 1000;
            maxLead = S(camCfg, 0x00);
            if (B(focusActor, 0x399) == 0) {
                lead = S(Scratchpad, 0x28A) + 0x30;
                S(Scratchpad, 0x28A) = lead;
                if (lead >= maxLead) {
                    S(Scratchpad, 0x28A) = maxLead;
                }
            } else {
                lead = S(Scratchpad, 0x28A) - 0x30;
                S(Scratchpad, 0x28A) = lead;
                if (lead <= -maxLead) {
                    S(Scratchpad, 0x28A) = -maxLead;
                }
            }

            ownerId = B(Scratchpad, 0x2A9);
            if (ownerId != 0 && H(Scratchpad, 0x2EE) == 0) {
                unsigned char *owner = D_800FF748 + (ownerId % 24) * 1000;
                tx = S(owner, 0x02) + S(Scratchpad, 0x28A);
                tz = S(owner, 0x06);
            } else {
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                tx = S(ball, 0x02) + S(Scratchpad, 0x28A);
                tz = S(ball, 0x06);
            }

            maxX = S(camCfg, 0x0E);
            if (tx > maxX) {
                tx = maxX;
            } else if (tx < -maxX) {
                tx = -maxX;
            }

            maxZ = S(camCfg, 0x0A);
            if (tz > maxZ) {
                tz = maxZ;
            } else {
                minZ = S(camCfg, 0x0C);
                if (tz < minZ) {
                    tz = minZ;
                }
            }

            SW(out, 0x0C) = (SW(Scratchpad, 0x1FC) * 4 + (int)tx) / 5;
            SW(out, 0x10) = (SW(Scratchpad, 0x200) * 4 + (int)S(camCfg, 0x02)) / 5;
            SW(out, 0x14) = (SW(Scratchpad, 0x204) * 4 + (int)tz) / 5;

            scaledX = (100 * (int)tx) / (int)B(camCfg, 0x10);
            SW(out, 0x00) = (SW(Scratchpad, 0x1F0) * 4 + scaledX) / 5;
            SW(out, 0x04) = (SW(Scratchpad, 0x1F4) * 4 + (int)S(camCfg, 0x06)) / 5;
            SW(out, 0x08) = (SW(Scratchpad, 0x1F8) * 4 + ((int)tz - (int)S(camCfg, 0x08))) / 5;
            break;
        }

        case 1:
        {
            unsigned char *ball;
            signed short maxX;
            signed short maxZ;
            signed short minZ;
            signed short pullback;
            signed short ballField14;
            int scaledX;
            int eyeZ;

            if (S(Scratchpad, 0x2EE) != 0) {
                S(Scratchpad, 0x2EA) = 0;
                return;
            }

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            tx = (S(ball, 0x02) + S(Scratchpad, 0x28A) + S(Scratchpad, 0x2B4)) >> 1;
            tz = (signed short)((S(ball, 0x06) + S(Scratchpad, 0x2B6)) / 2);

            maxX = S(camCfg, 0x0E);
            if (tx > maxX) {
                tx = maxX;
            } else if (tx < -maxX) {
                tx = -maxX;
            }

            maxZ = S(camCfg, 0x0A);
            if (tz > maxZ) {
                tz = maxZ;
            } else {
                minZ = S(camCfg, 0x0C);
                if (tz < minZ) {
                    tz = minZ;
                }
            }

            SW(out, 0x0C) = (SW(Scratchpad, 0x1FC) * 4 + (int)tx) / 5;
            SW(out, 0x10) = (SW(Scratchpad, 0x200) + (int)S(camCfg, 0x02)) >> 1;
            SW(out, 0x14) = (SW(Scratchpad, 0x204) * 3 + (int)tz * 2) / 5;

            scaledX = (100 * (int)tx) / (int)B(camCfg, 0x10);
            SW(out, 0x00) = (SW(Scratchpad, 0x1F0) * 4 + scaledX) / 5;
            SW(out, 0x04) = (SW(Scratchpad, 0x1F4) * 9 + (int)S(camCfg, 0x06)) / 10;

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            pullback = S(Scratchpad, 0x2EA);
            ballField14 = S(ball, 0x14);
            if (ballField14 < 0) {
                ballField14 = -ballField14;
            }
            tz = (signed short)(tz - (signed short)H(camCfg, 0x08) - pullback);
            if (ballField14 >= 0x91 && B(Scratchpad, 0x2A0) == 0) {
                S(Scratchpad, 0x2EA) = pullback + 0x60;
            }

            eyeZ = (SW(Scratchpad, 0x1F8) * 3 + (int)tz * 2) / 5;
            SW(out, 0x08) = eyeZ;
            /* UNCLEAR: unsigned comparison with -0x4A00 (0xFFFFB600); clamps if eyeZ >= 0 or eyeZ < -18944 */
            if ((unsigned int)eyeZ < (unsigned int)-0x4A00) {
                SW(out, 0x08) = -0x4A00;
            }
            break;
        }

        case 3:
        {
            unsigned char phase = B(Scratchpad, 0x29A);
            unsigned char *ball;
            signed short maxX;
            signed short maxZ;
            signed short minZ;

            if (phase == 3 || phase == 0) {
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                tx = S(ball, 0x02);
            } else {
                unsigned char *actor = D_800FF748 + (B(Scratchpad, 0x2AD) % 24) * 1000;
                signed short lead = S(camCfg, 0x00);
                if (B(actor, 0x399) != 0) {
                    lead = -lead;
                }
                S(Scratchpad, 0x28A) = lead;
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                tx = S(ball, 0x02) + lead;
            }

            tz = S(((unsigned char *)W(Scratchpad, 0x2F4)), 0x06);

            maxX = S(camCfg, 0x0E);
            if (tx > maxX) {
                tx = maxX;
            } else if (tx < -maxX) {
                tx = -maxX;
            }

            maxZ = S(camCfg, 0x0A);
            if (tz > maxZ) {
                tz = maxZ;
            } else {
                minZ = S(camCfg, 0x0C);
                if (tz < minZ) {
                    tz = minZ;
                }
            }

            SW(out, 0x0C) = (int)tx;
            SW(out, 0x14) = (int)tz;
            SW(out, 0x10) = (int)S(camCfg, 0x02);
            SW(out, 0x00) = (100 * (int)tx) / (int)B(camCfg, 0x10);
            SW(out, 0x04) = (int)S(camCfg, 0x06);
            SW(out, 0x08) = (int)tz - (int)S(camCfg, 0x08);
            S(Scratchpad, 0x2EA) = 0;
            break;
        }
        }
        return;
    }

    if (camMode >= 8) {
        return;
    }

    /* Camera modes 5..7 (side / touchline-oriented camera) */
    B(Scratchpad, 0x1EC) = 3;
    switch (B(Scratchpad, 0x2E2)) {
    case 1:
    case 4:
        sideDir = 1;
        B(Scratchpad, 0x2E7) = 0;
        break;
    default:
        if (B(Scratchpad, 0x2E0) != 0) {
            sideDir = -1;
            B(Scratchpad, 0x2E7) = 0x80;
        } else {
            sideDir = 1;
            B(Scratchpad, 0x2E7) = 0;
        }
        break;
    }

    switch (B(Scratchpad, 0x2F2)) {
    case 0:
    {
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        signed short maxZ = S(camCfg, 0x0A);
        signed short minZ;
        signed short ballZ = S(ball, 0x06);

        tx = S(ball, 0x02);
        if (ballZ > maxZ) {
            tz = maxZ;
        } else {
            minZ = S(camCfg, 0x0C);
            tz = (ballZ < minZ) ? minZ : ballZ;
        }
        break;
    }

    case 1:
    {
        unsigned char *ball;
        signed short midZ;
        signed short maxZ;
        signed short minZ;

        if (H(Scratchpad, 0x2EE) != 0) {
            return;
        }
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        tx = (signed short)((S(ball, 0x02) + S(Scratchpad, 0x2B4)) >> 1);
        midZ = (signed short)((S(ball, 0x06) + S(Scratchpad, 0x2B6)) >> 1);

        maxZ = S(camCfg, 0x0A);
        if (midZ > maxZ) {
            tz = maxZ;
        } else {
            minZ = S(camCfg, 0x0C);
            tz = (midZ < minZ) ? minZ : midZ;
        }
        break;
    }

    case 3:
    {
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        signed short ballZ = S(ball, 0x06);
        signed short maxZ = S(camCfg, 0x0A);
        signed short minZ;

        if (ballZ > maxZ) {
            tz = maxZ;
        } else {
            minZ = S(camCfg, 0x0C);
            tz = (ballZ < minZ) ? minZ : ballZ;
        }

        SW(out, 0x0C) = (int)S(ball, 0x02);
        SW(out, 0x14) = (int)tz;
        SW(out, 0x10) = (int)S(ball, 0x04) + (int)S(camCfg, 0x02);
        SW(out, 0x00) = (int)S(ball, 0x02) + (int)(signed short)sideDir * (int)S(camCfg, 0x04);
        SW(out, 0x08) = (int)tz;
        SW(out, 0x04) = (int)S(camCfg, 0x06);
        return;
    }

    default:
        return;
    }

    SW(out, 0x0C) = (SW(Scratchpad, 0x1FC) * 4 + (int)tx) / 5;
    SW(out, 0x10) = (SW(Scratchpad, 0x200) * 4 + (int)S(camCfg, 0x02)) / 5;
    SW(out, 0x14) = (SW(Scratchpad, 0x204) * 4 + (int)tz) / 5;
    SW(out, 0x00) = (SW(Scratchpad, 0x1F0) * 4 + ((int)tx + (int)(signed short)sideDir * (int)S(camCfg, 0x04))) / 5;
    SW(out, 0x04) = (SW(Scratchpad, 0x1F4) * 4 + (int)S(camCfg, 0x06)) / 5;
    SW(out, 0x08) = (SW(Scratchpad, 0x1F8) * 4 + (int)tz) / 5;
}
