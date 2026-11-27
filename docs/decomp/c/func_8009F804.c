/*
 * func_8009F804 - AI wide / off-ball positioning target adjustment
 *
 * Purpose:
 *   Computes an AI player's target (X, Z) coordinates in Scratchpad+0x0F8/0x0FC
 *   by calling func_8005A85C for the base formation position and then adjusting
 *   the lateral Z and depth X targets based on the ball's position, player
 *   index/tag (+0x3DA % 10), team X reference (+0x30C[2]), and wide flag (arg1).
 *   When the ball is deep in either half (|ball_x| >= 0x23E3), clamps X to
 *   +/-0x1DE2 and constrains Z to +/-0x12F4 on the ball's side.
 *
 * Arguments:
 *   actor (unsigned char *): pointer to 1000-byte player actor struct.
 *   flag  (unsigned int)   : lateral side/inversion flag (tested as (flag & 0xFF)).
 *
 * Return value:
 *   void.
 *
 * Scratchpad (0x1F800000) reads/writes:
 *   - 0x0F8 (s16, R/W): target X coordinate.
 *   - 0x0FC (s16, R/W): target Z coordinate.
 *   - 0x2F4 (u8 *, R): pointer to ball actor (+0x02 s16 x, +0x06 s16 z).
 *   - 0x30C (s16[2], R): per-attack-direction X reference line.
 *
 * Actor fields read:
 *   - +0x399 (u8, R): attack direction flag (0 = +X, 1 = -X).
 *   - +0x3DA (u8, R): UNCLEAR exact role (used modulo 10; < 4 selects 0x1892 vs 0x1B61).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x8B6  (2230 raw = 8.92 m): ball |Z| threshold.
 *   - 0x12F4 (4852 raw = 19.408 m): wide Z target offset.
 *   - 0x1892 (6290 raw = 25.16 m): wider Z target offset when (+0x3DA % 10) < 4.
 *   - 0x1B61 (7009 raw = 28.036 m): outer wing Z target offset.
 *   - 0x200  (512 raw = 2.048 m): goal-side X pullback from Scratchpad+0x30C.
 *   - 0x500  (1280 raw = 5.12 m): minimum X separation threshold from ball X.
 *   - 0x400  (1024 raw = 4.096 m): forward X offset ahead of ball X.
 *   - 0x23E3 (9187 raw = 36.748 m): deep-half ball |X| threshold.
 *   - 0x1DE2 (7650 raw = 30.6 m): clamped X target when |ball_x| >= 0x23E3.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern void func_8005A85C(unsigned char *actor, signed short *outX, signed short *outZ);
extern void func_800A0B60(unsigned char *actor, signed short *outX, signed short *outZ);

void func_8009F804(unsigned char *actor, unsigned int flag)
{
    signed short *outX = (signed short *)(Scratchpad + 0x0F8);
    signed short *outZ = (signed short *)(Scratchpad + 0x0FC);
    unsigned char *ball;
    signed short dir_sign;
    signed short ball_z;
    int abs_ball_z;
    int target_z_sign;
    int ball_z_sign;
    unsigned int flag_byte;
    int use_wide_path = 0;

    func_8005A85C(actor, outX, outZ);

    dir_sign = (B(actor, 0x399) == 0) ? 1 : -1;
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    ball_z = S(ball, 0x6);
    abs_ball_z = (ball_z >= 0) ? ball_z : -ball_z;
    flag_byte = flag & 0xFFu;

    if (abs_ball_z >= 0x8B6) {
        target_z_sign = (*outZ >= 0) ? 1 : -1;
        ball_z_sign = (ball_z >= 0) ? 1 : -1;
        if (target_z_sign == ball_z_sign) {
            use_wide_path = 1;
        }
    }

    if (!use_wide_path) {
        int sum;
        signed short z_val = (signed short)(dir_sign * 0x12F4);
        if (flag_byte != 0) {
            z_val = (signed short)(-z_val);
        }
        *outZ = z_val;

        sum = S(ball, 0x2) + S(Scratchpad, 0x30C + (B(actor, 0x399) << 1));
        *outX = (signed short)((sum + (int)((unsigned int)sum >> 31)) >> 1);
    } else {
        /* UNCLEAR: meaning of actor+0x3DA (used mod 10 to select 0x1892 vs 0x1B61) */
        unsigned int tag_mod10 = B(actor, 0x3DA) % 10u;
        signed short ref_x;
        signed short ball_x;
        int diff_x;
        int abs_diff_x;
        int abs_tx;
        int abs_bx;

        if (tag_mod10 < 4u) {
            signed short z_val = (signed short)(dir_sign * 0x1892);
            if (flag_byte != 0) {
                z_val = (signed short)(-z_val);
            }
            *outZ = z_val;
        } else {
            *outZ = (*outZ >= 0) ? 0x1B61 : -0x1B61;
        }

        ref_x = S(Scratchpad, 0x30C + (B(actor, 0x399) << 1));
        *outX = (signed short)(ref_x - (dir_sign << 9));

        ball_x = S(ball, 0x2);
        diff_x = (int)*outX - (int)ball_x;
        abs_diff_x = (diff_x >= 0) ? diff_x : -diff_x;
        if (abs_diff_x < 0x500) {
            *outX = (signed short)(ball_x + (dir_sign << 10));
        }

        abs_tx = (*outX >= 0) ? *outX : -*outX;
        abs_bx = (ball_x >= 0) ? ball_x : -ball_x;
        if (abs_bx < abs_tx) {
            func_800A0B60(actor, outX, outZ);
        }
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    {
        signed short bx = S(ball, 0x2);
        int abs_bx = (bx >= 0) ? bx : -bx;
        if (abs_bx >= 0x23E3) {
            signed short bz = S(ball, 0x6);
            int bz_sign = (bz >= 0) ? 1 : -1;
            int tz_sign = (*outZ >= 0) ? 1 : -1;

            *outX = (signed short)(dir_sign * 0x1DE2);
            if (bz_sign == tz_sign) {
                *outZ = (*outZ >= 0) ? 0x12F4 : -0x12F4;
            }
        }
    }
}
