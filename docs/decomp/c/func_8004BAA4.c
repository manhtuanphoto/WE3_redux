/*
 * func_8004BAA4 - Ball physics & trajectory prediction step
 *
 * Purpose:
 *   Advances the ball simulation state stored in scratchpad (0xB0..0xBC for
 *   state/heading/spin/speed/bounce type, 0xF8/0xFA/0xFC for predicted xyz).
 *   Handles airborne flight, ground bounce damping, gravity, quadratic air
 *   drag, rolling friction, and ball-carried dribbling movement.
 *
 * Arguments:
 *   step_idx (u16 in $a0) - simulation step/frame index used for spin-curve timing.
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Actor fields read & written:
 *   Scratch 0xB0 (u8)  - physics mode: 0=idle/check, 1=airborne, 2=rolling, 3=carried, >=4=stop
 *   Scratch 0xB1 (u8)  - movement heading angle (0..255 = 360 deg)
 *   Scratch 0xB2 (u8)  - curve/spin period in steps (0 = no curve; +2 per bounce)
 *   Scratch 0xB3 (u8)  - curve direction (nonzero = +1, zero = -1)
 *   Scratch 0xB4 (u32) - horizontal speed (<< 8 fixed-point; 250 raw = 1 m, 30 Hz)
 *   Scratch 0xB8 (s32) - vertical velocity (<< 8 fixed-point; negative = upward)
 *   Scratch 0xBC (u8)  - flight/bounce type (0..3)
 *   Scratch 0xF8/0xFA/0xFC (s16) - predicted ball x/y/z (-32 = -0.128 m ground plane)
 *   Scratch 0x2A9 (u8) - ball owner actor id (used in mode 3)
 *   Scratch 0x2F4 (ptr)- ball actor pointer (reads +0x3A4 move heading in mode 3)
 *   Actor +0x3A0 (s32) - owner speed (read in mode 3)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   -32 (-0x20)     = -0.128 m (ball ground contact height y)
 *   -17663 (-0x44FF)= ~ -69.0 raw/frame << 8 (~ -8.28 m/s upward after bounce)
 *   -1471 (-0x5BF)  = ~ -5.75 raw/frame << 8 (~ -0.69 m/s bounce cutoff to roll)
 *   0x2E0/2C0/320/338 = 736/704/800/824 gravity per frame (<< 8) by flight type 0..3
 *   0x100 (256)     = 1.0 raw/frame << 8 (0.12 m/s rolling stop threshold)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char D_800FF748[];

extern int func_8001E694(unsigned int angle, int scale);
extern int func_8001E5A4(unsigned int angle, int scale);
extern unsigned int func_800B1128(unsigned int val);

void func_8004BAA4(unsigned int step_idx)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char state = B(scratch, 0xB0);

    switch (state) {
    case 0:
        if (SW(scratch, 0xB8) != 0) {
            B(scratch, 0xB0) = 1;
        } else if (S(scratch, 0xFA) == -32) {
            B(scratch, 0xB0) = 2;
        } else {
            B(scratch, 0xB0) = 1;
        }
        break;

    case 1: {
        unsigned int spin_period = B(scratch, 0xB2);
        int cos_dx;
        int sin_dz;
        unsigned int spd_mag;
        int h_spd;
        int v_vel;

        if (spin_period != 0 && ((unsigned short)step_idx % (int)spin_period) == 0) {
            if (B(scratch, 0xB3) != 0) {
                B(scratch, 0xB1) = (unsigned char)(B(scratch, 0xB1) + 1);
            } else {
                B(scratch, 0xB1) = (unsigned char)(B(scratch, 0xB1) - 1);
            }
        }

        cos_dx = func_8001E694(B(scratch, 0xB1), SW(scratch, 0xB4));
        S(scratch, 0xF8) = (signed short)(S(scratch, 0xF8) + (cos_dx >> 8));
        S(scratch, 0xFA) = (signed short)(S(scratch, 0xFA) + (SW(scratch, 0xB8) >> 8));
        sin_dz = func_8001E5A4(B(scratch, 0xB1), SW(scratch, 0xB4));
        S(scratch, 0xFC) = (signed short)(S(scratch, 0xFC) + (sin_dz >> 8));

        if (S(scratch, 0xFA) < -32) {
            /* Airborne: apply gravity according to flight type 0..3 */
            switch (B(scratch, 0xBC)) {
            case 0:
                SW(scratch, 0xB8) += 0x2E0;
                break;
            case 1:
                SW(scratch, 0xB8) += 0x2C0;
                break;
            case 2:
                SW(scratch, 0xB8) += 0x320;
                break;
            case 3:
                SW(scratch, 0xB8) += 0x338;
                break;
            }
        } else {
            /* Ground contact: compute bounce damping */
            int abs_vy_plus1 = SW(scratch, 0xB8) + 1;
            int ratio;
            unsigned int bounce_type;
            int damp;

            if (abs_vy_plus1 < 0) {
                abs_vy_plus1 = -abs_vy_plus1;
            }
            ratio = (int)(W(scratch, 0xB4) / (unsigned int)abs_vy_plus1);
            if (ratio >= 9) {
                ratio = 8;
            }

            bounce_type = B(scratch, 0xBC);
            switch (bounce_type) {
            case 0:
                damp = ratio << 2;
                h_spd = SW(scratch, 0xB4);
                v_vel = SW(scratch, 0xB8);
                W(scratch, 0xB4) = (unsigned int)(h_spd - ((unsigned int)(h_spd * (0x40 - damp)) >> 8));
                SW(scratch, 0xB8) = v_vel - ((v_vel * (0x60 - damp)) / 256);
                break;
            case 1:
                damp = ratio << 2;
                h_spd = SW(scratch, 0xB4);
                v_vel = SW(scratch, 0xB8);
                W(scratch, 0xB4) = (unsigned int)(h_spd - ((unsigned int)(h_spd * (0x40 - damp)) >> 7));
                SW(scratch, 0xB8) = v_vel - ((v_vel * (0x40 - damp)) / 240);
                break;
            case 2:
                damp = ratio << 2;
                h_spd = SW(scratch, 0xB4);
                v_vel = SW(scratch, 0xB8);
                B(scratch, 0xBC) = 0;
                W(scratch, 0xB4) = (unsigned int)(h_spd - ((unsigned int)(h_spd * (0x40 - damp)) / 272U));
                SW(scratch, 0xB8) = v_vel - ((v_vel * (0x60 - damp)) / 288);
                break;
            case 3:
                damp = ratio << 2;
                h_spd = SW(scratch, 0xB4);
                v_vel = SW(scratch, 0xB8);
                B(scratch, 0xBC) = 0;
                W(scratch, 0xB4) = (unsigned int)(h_spd - ((unsigned int)(h_spd * (0x40 - damp)) / 288U));
                SW(scratch, 0xB8) = v_vel - ((v_vel * (0x60 - damp)) / 304);
                break;
            }

            S(scratch, 0xFA) = -32;
            SW(scratch, 0xB8) = -SW(scratch, 0xB8);
            if (B(scratch, 0xB2) != 0) {
                B(scratch, 0xB2) = (unsigned char)(B(scratch, 0xB2) + 2);
            }
            if (SW(scratch, 0xB8) >= -0x44FF) {
                B(scratch, 0xBC) = 0;
            }
            if (SW(scratch, 0xB8) >= -0x5BF) {
                SW(scratch, 0xB8) = 0;
                B(scratch, 0xB0) = 2;
            }
        }

        /* Quadratic air drag proportional to speed magnitude */
        spd_mag = func_800B1128((unsigned int)(SW(scratch, 0xB4) * SW(scratch, 0xB4) +
                                               SW(scratch, 0xB8) * SW(scratch, 0xB8)));
        h_spd = SW(scratch, 0xB4);
        SW(scratch, 0xB4) = h_spd - (int)(((unsigned int)(spd_mag * (unsigned int)h_spd) >> 21) / 7U);
        v_vel = SW(scratch, 0xB8);
        SW(scratch, 0xB8) = v_vel - ((int)spd_mag * v_vel) / 14680064;
        break;
    }

    case 2: {
        int cos_dx;
        int sin_dz;
        unsigned int spd;

        S(scratch, 0xFA) = -32;
        cos_dx = func_8001E694(B(scratch, 0xB1), SW(scratch, 0xB4));
        S(scratch, 0xF8) = (signed short)(S(scratch, 0xF8) + (cos_dx >> 8));
        sin_dz = func_8001E5A4(B(scratch, 0xB1), SW(scratch, 0xB4));

        spd = W(scratch, 0xB4);
        spd -= (spd >> 6);
        B(scratch, 0xB2) = 0;
        S(scratch, 0xFC) = (signed short)(S(scratch, 0xFC) + (sin_dz >> 8));

        spd -= ((unsigned int)(spd * spd) >> 21) / 7U;
        W(scratch, 0xB4) = spd;
        if (spd < 0x100U) {
            B(scratch, 0xB0) = 0;
        }
        break;
    }

    case 3: {
        unsigned int owner_idx = B(scratch, 0x2A9) % 24U;
        unsigned char *owner = D_800FF748 + owner_idx * 1000;
        unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
        unsigned int heading = B(ball, 0x3A4);
        int dx = func_8001E694(heading, SW(owner, 0x3A0));
        int dz;

        S(scratch, 0xF8) = (signed short)(S(scratch, 0xF8) + dx);
        dz = func_8001E5A4(heading, SW(owner, 0x3A0));
        S(scratch, 0xFC) = (signed short)(S(scratch, 0xFC) + dz);
        /* Fall through to reset height/speed */
    }
    case 4:
    default:
        S(scratch, 0xFA) = -32;
        W(scratch, 0xB4) = 0;
        break;
    }
}
