/*
 * func_8002F2B0
 *
 * Purpose:
 *   Iterative ball-trajectory power solver for passes/crosses. Sweeps kick speed
 *   from `cap_pwr << 8` upward in steps of `0x400` (`4 << 8`) until `min_pwr`
 *   (or until a valid trajectory is found). For each candidate speed, copies the
 *   ball position to Scratchpad+0xF8..0xFC, configures simulated ball velocity at
 *   Scratchpad+0xB0..0xB8 (vertical lift `-736 * lift`), steps the ball physics
 *   via func_8004BAA4(0) until the simulated ball covers squared distance `dist^2`,
 *   and checks if the target player (moving at `max_spd`) can arrive at the
 *   simulated landing point in `<= s1` (ball travel) frames (with `>= 16` total
 *   frames including `lead_steps`). Stores the final landing X/Z to
 *   Scratchpad+0xFE/0x100 and step count `s1` to ball+0x3CC.
 *
 * Arguments:
 *   target     (unsigned char *) - target teammate actor pointer
 *   heading    (unsigned int)    - kick direction byte (0..255)
 *   min_pwr    (unsigned int)    - upper bound / sweep limit for shifted power
 *   max_pwr    (unsigned int)    - max remaining ball speed allowed at landing
 *   max_spd    (int)             - receiver speed used to compute arrival frames
 *   cap_pwr    (int)             - initial power (shifted left by 8)
 *   dist       (int)             - target travel distance (squared internally)
 *   lead_steps (int)             - signed 16-bit frame offset added to receiver ETA
 *   lift       (int)             - vertical loft level (vy0 = -736 * lift)
 *
 * Return value:
 *   unsigned int - solved unshifted kick power (`power_fp >> 8`).
 *
 * Scratchpad (0x1F800000) / Actor fields read & written:
 *   Scratchpad +0xB0 (u8)  - sim ball flight mode (1 if vy0 != 0, else 2)
 *   Scratchpad +0xB1 (u8)  - sim ball heading
 *   Scratchpad +0xB2 (u8)  - cleared to 0
 *   Scratchpad +0xB4 (u32) - sim ball horizontal speed (stops if < 0x201)
 *   Scratchpad +0xB8 (s32) - sim ball vertical velocity (-736 * lift)
 *   Scratchpad +0xF8/+0xFA/+0xFC (s16) - sim ball X/Y/Z
 *   Scratchpad +0xFE/+0x100 (u16) - solved landing X/Z
 *   Scratchpad +0x2F4 (ptr) - ball actor (+0x02/0x04/0x06 pos, +0x3CC step count)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCHPAD ((unsigned char *)0x1F800000)

extern void func_8004BAA4(int mode);
extern unsigned int func_800B1128(unsigned int val_sq);

unsigned int func_8002F2B0(
    unsigned char *target,
    unsigned int heading,
    unsigned int min_pwr,
    unsigned int max_pwr,
    int max_spd,
    int cap_pwr,
    int dist,
    int lead_steps,
    int lift)
{
    unsigned char *scratch = SCRATCHPAD;
    unsigned int power_fp = (unsigned int)cap_pwr << 8;
    unsigned int dist_sq = (unsigned int)(dist * dist);
    int vy0;
    int steps = 0; /* Uninitialized on early-exit if power_fp >= min_pwr; matches asm $s1 */

    if (power_fp < min_pwr) {
        vy0 = -(lift * 736);

        do {
            unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
            int sim_x;
            int sim_z;
            int dx;
            int dz;
            unsigned int travelled_sq;

            H(scratch, 0xF8) = H(ball, 0x02);
            H(scratch, 0xFA) = H(ball, 0x04);
            H(scratch, 0xFC) = H(ball, 0x06);
            B(scratch, 0xB2) = 0;
            W(scratch, 0xB4) = power_fp;
            SW(scratch, 0xB8) = vy0;
            B(scratch, 0xB1) = (unsigned char)heading;
            B(scratch, 0xB0) = (vy0 != 0) ? 1 : 2;

            sim_x = S(scratch, 0xF8);
            dx = S(ball, 0x02) - sim_x;
            dz = S(ball, 0x06) - S(scratch, 0xFC);
            travelled_sq = (unsigned int)(dx * dx + dz * dz);
            steps = 0;

            while (travelled_sq < dist_sq) {
                int abs_x;

                if (W(scratch, 0xB4) < 0x201U) {
                    break;
                }
                abs_x = (sim_x < 0) ? -sim_x : sim_x;
                if (abs_x >= 0x3301) {
                    break;
                }

                func_8004BAA4(0);
                ball = (unsigned char *)W(scratch, 0x2F4);
                sim_x = S(scratch, 0xF8);
                dx = S(ball, 0x02) - sim_x;
                dz = S(ball, 0x06) - S(scratch, 0xFC);
                travelled_sq = (unsigned int)(dx * dx + dz * dz);
                steps++;
            }

            if (W(scratch, 0xB4) < max_pwr) {
                int abs_x;

                sim_x = S(scratch, 0xF8);
                abs_x = (sim_x < 0) ? -sim_x : sim_x;
                if (abs_x < 0x3300) {
                    unsigned int rem_dist;
                    int eta;

                    sim_z = S(scratch, 0xFC);
                    dx = S(target, 0x02) - sim_x;
                    dz = S(target, 0x06) - sim_z;
                    rem_dist = func_800B1128((unsigned int)(dx * dx + dz * dz));
                    eta = (signed short)lead_steps + (int)(rem_dist / (unsigned int)max_spd);
                    if (eta >= 0x10 && eta >= steps) {
                        break;
                    }
                }
            }

            power_fp += 0x400U;
        } while (power_fp < min_pwr);
    }

    H(scratch, 0xFE) = H(scratch, 0xF8);
    H(scratch, 0x100) = H(scratch, 0xFC);
    H((unsigned char *)W(scratch, 0x2F4), 0x3CC) = (unsigned short)steps;

    return power_fp >> 8;
}
