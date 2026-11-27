/*
 * func_8003CEE4 - Ball interception / reachability check at predicted frame
 *
 * Purpose:
 *   Checks whether a player actor can reach or intercept the predicted ball
 *   position at lookahead `frameIdx` (Scratchpad+0x36/0x4E/0x66) relative to
 *   the actor's reach/contact point (Scratchpad+0xF0/0xF2/0xF4). Verifies open
 *   play, ball height ceiling, ownership/cooldowns, closest-approach timing
 *   (distance increasing at frameIdx -> frameIdx+1), jump/vertical reach from
 *   player stat (D_8010C330 >> 11 & 0x7F), and speed threshold (+0x3B4).
 *
 * Arguments:
 *   actor     (unsigned char *): pointer to 1000-byte player actor struct.
 *   frameIdx  (int)            : predicted ball trajectory frame index (0..).
 *   checkY    (int)            : height-check mode (1 = strict Y proximity when close; nonzero = Y ceiling check).
 *   mode      (int)            : 0 = disallow far path; 0xFF = skip possession/speed check; other = full check.
 *   maxDistSq (unsigned int)   : maximum squared XZ distance from reach point to predicted ball.
 *
 * Return value:
 *   int: 1 if reachable/interceptable, 0 otherwise.
 *
 * Scratchpad (0x1F800000) reads:
 *   - 0x036.. (s16[]): predicted ball X table (indexed by frameIdx * 2).
 *   - 0x04E.. (s16[]): predicted ball Y table (indexed by frameIdx * 2; negative = up).
 *   - 0x066.. (s16[]): predicted ball Z table (indexed by frameIdx * 2).
 *   - 0x0F0/0x0F2/0x0F4 (s16): reach/contact point X, Y, Z.
 *   - 0x29A (u8): match phase (must be 1 = open play).
 *   - 0x2AA (u8): previous ball owner ID.
 *   - 0x2B0 (u32): ball event code (0xB bypasses timer check).
 *   - 0x2CF (u8[2]): per-side possession flags.
 *   - 0x2F4 (u8 *): pointer to ball actor (+0x3A0 u32 speed, +0x3DC s16 state).
 *   - 0x31C (s16): frames since last touch (< 7 blocks interception unless prev owner or event 0xB).
 *
 * Actor fields read:
 *   - +0x02/+0x06 (s16): actor X, Z position.
 *   - +0x38D (u8): actor ID.
 *   - +0x392 (u8): team slot (stride 12 in D_8010C330).
 *   - +0x398 (u8): side (0/1, stride 264 in D_8010C330).
 *   - +0x399 (u8): attack direction flag (0/1).
 *   - +0x3B4 (u8): interception speed threshold compared against (ball_speed >> 8).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - -0x330 (-816 raw = 3.264 m height): max ball altitude at frameIdx.
 *   - 0x2401 (9217 = ~96^2 raw): close XZ distance^2 threshold (~0.384 m).
 *   - 0x80 (128 raw = 0.512 m): vertical proximity when close and checkY == 1.
 *   - 0x60 (96 raw = 0.384 m): vertical margin above reachY when checkY != 0.
 *   - 0x200 (512 raw = 2.048 m): max reachY - ballY gap.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_8010C330[];

int func_8003CEE4(unsigned char *actor, int frameIdx, int checkY, int mode, unsigned int maxDistSq)
{
    signed short signed_x;
    unsigned int phase;
    unsigned int f;
    unsigned char *ball;
    unsigned char *f_ptr;
    signed short rx, rz;
    signed short bx0, bz0, bx1, bz1;
    unsigned short by0_u16;
    unsigned int dist_sq0, dist_sq1;
    int p_dist_sq0, p_dist_sq1;
    unsigned int tbl_off;
    int jump_stat;
    int max_reach_y;
    unsigned int poss;

    signed_x = S(actor, 0x2);
    if (B(actor, 0x399) != 0) {
        signed_x = (signed short)(-signed_x);
    }

    phase = B(Scratchpad, 0x29A);
    if (phase != 1u) {
        return 0;
    }

    f = (unsigned int)frameIdx & 0xFFu;
    if (S(Scratchpad, 0x4E + (f << 1)) < -0x330) {
        return 0;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    if (S(ball, 0x3DC) == 2 && B(Scratchpad, 0x2CF + B(actor, 0x398)) == phase) {
        /* Note: exact bounds from asm (-0x23E1 and -0x3020); always fails since -0x23E1 > -0x3020 */
        if ((int)signed_x < -0x23E1 || (int)signed_x >= -0x3020) {
            return 0;
        }
    }

    if (B(Scratchpad, 0x2AA) != B(actor, 0x38D)) {
        if (W(Scratchpad, 0x2B0) != 0xBu && S(Scratchpad, 0x31C) < 7) {
            return 0;
        }
    }

    f_ptr = Scratchpad + (f << 1);
    rx = S(Scratchpad, 0x0F0);
    bx0 = S(f_ptr, 0x036);
    rz = S(Scratchpad, 0x0F4);
    bz0 = S(f_ptr, 0x066);
    dist_sq0 = (unsigned int)((rx - bx0) * (rx - bx0) + (rz - bz0) * (rz - bz0));

    bx1 = S(f_ptr, 0x038);
    bz1 = S(f_ptr, 0x068);
    dist_sq1 = (unsigned int)((rx - bx1) * (rx - bx1) + (rz - bz1) * (rz - bz1));
    by0_u16 = H(f_ptr, 0x04E);

    if (dist_sq0 < 0x2401u) {
        if ((checkY & 0xFF) != 1) {
            return 1;
        } else {
            int dy = (int)S(Scratchpad, 0x0F2) - (int)(signed short)by0_u16;
            if (dy < 0) {
                dy = -dy;
            }
            return (dy < 0x80) ? 1 : 0;
        }
    }

    if ((mode & 0xFF) == 0) {
        return 0;
    }
    if (maxDistSq < dist_sq0) {
        return 0;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    if ((W(ball, 0x3A0) >> 8) < 0x30u) {
        return 0;
    }

    if ((checkY & 0xFF) != 0) {
        if ((int)(signed short)by0_u16 >= (int)S(Scratchpad, 0x0F2) + 0x60) {
            return 0;
        }
    }

    if (dist_sq0 >= dist_sq1) {
        return 0;
    }

    {
        signed short px = S(actor, 0x2);
        signed short pz = S(actor, 0x6);
        int dx0 = (int)px - (int)bx0;
        int dz0 = (int)pz - (int)bz0;
        int dx1 = (int)px - (int)S(f_ptr, 0x038);
        int dz1 = (int)pz - (int)S(f_ptr, 0x068);
        p_dist_sq0 = dx0 * dx0 + dz0 * dz0;
        p_dist_sq1 = dx1 * dx1 + dz1 * dz1;
        if (p_dist_sq0 >= p_dist_sq1) {
            return 0;
        }
    }

    tbl_off = B(actor, 0x392) * 12u + B(actor, 0x398) * 264u;
    jump_stat = (int)((W(D_8010C330, tbl_off) >> 11) & 0x7Fu) + 0x64;
    /* (-jump_stat * 592) / 180 - 256 */
    max_reach_y = (-(jump_stat * 592)) / 180 - 0x100;
    if (max_reach_y >= (int)(signed short)by0_u16) {
        return 0;
    }

    if ((int)S(Scratchpad, 0x0F2) - (int)(signed short)by0_u16 >= 0x200) {
        return 0;
    }

    if ((mode & 0xFF) == 0xFF) {
        return 1;
    }

    poss = B(Scratchpad, 0x2CF + B(actor, 0x398));
    if (poss != 0u) {
        return 0;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    if (B(actor, 0x3B4) < (W(ball, 0x3A0) >> 8)) {
        return 0;
    }

    return 1;
}
