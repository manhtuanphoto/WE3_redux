/*
 * func_800498AC
 *
 * Purpose:
 *   Goalpost and goal-net collision clamping for a player/entity. Calls
 *   func_8005C1CC(actor, 0) and takes the larger |x| between actor+0x2 and
 *   Scratchpad+0xF0 (along with corresponding z at actor+0x6 or Scratchpad+0xF4).
 *   When |x| >= 0x32E1 (goal line):
 *   - Near the left goalpost (|z + 0x3C0| < 0xC0) or right goalpost
 *     (|z - 0x3C0| < 0xC0): if |x| >= 0x3368, pushes actor->z out of the post
 *     (±0x2FF / ±0x481); otherwise clamps actor->x back to ±0x32E0.
 *   - Between the goalposts (|z| < 0x3C0) and past the back of the net
 *     (|x| >= 0x3555): clamps actor->x back to ±0x3554.
 *
 * Arguments:
 *   actor - pointer to actor/entity struct (s16 x at +0x2, s16 z at +0x6)
 *
 * Return value:
 *   None (void)
 *
 * Scratchpad (0x1F800000) fields read / written:
 *   +0x0F0 (s16) alternate/projected x (overwritten with |Scratchpad+0xF0|)
 *   +0x0F4 (u16/s16) alternate/projected z
 */

#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define SH(p, o) (*(short *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern void func_8005C1CC(unsigned char *actor, unsigned int arg1);

void func_800498AC(unsigned char *actor)
{
    short abs_actor_x;
    short abs_sp_x;
    short max_x;
    unsigned short cur_z;
    short z;

    abs_actor_x = SH(actor, 0x02);
    if (abs_actor_x < 0) {
        abs_actor_x = (short)(-abs_actor_x);
    }

    func_8005C1CC(actor, 0);

    abs_sp_x = SH(Scratchpad, 0x0F0);
    if (abs_sp_x < 0) {
        abs_sp_x = (short)(-abs_sp_x);
    }
    SH(Scratchpad, 0x0F0) = abs_sp_x;

    if (abs_sp_x < abs_actor_x) {
        max_x = abs_actor_x;
        cur_z = H(actor, 0x06);
    } else {
        max_x = abs_sp_x;
        cur_z = H(Scratchpad, 0x0F4);
    }

    if (max_x < 0x32E1) {
        return;
    }

    z = (short)cur_z;
    if (((z + 0x3C0) >= 0 ? (z + 0x3C0) : -(z + 0x3C0)) < 0xC0) {
        if (max_x >= 0x3368) {
            short az = SH(actor, 0x06);
            int target_z = (az < -0x3BF) ? ((int)az - 0x481) : ((int)az - 0x2FF);
            SH(actor, 0x06) = (short)(target_z - (int)cur_z);
        } else {
            short ax = SH(actor, 0x02);
            SH(actor, 0x02) = (ax > 0)
                ? (short)((int)ax + 0x32E0 - (int)max_x)
                : (short)((int)max_x + (int)ax - 0x32E0);
        }
    } else if (((z - 0x3C0) >= 0 ? (z - 0x3C0) : -(z - 0x3C0)) < 0xC0) {
        if (max_x >= 0x3368) {
            short az = SH(actor, 0x06);
            int target_z = (az < 0x3C1) ? ((int)az + 0x2FF) : ((int)az + 0x481);
            SH(actor, 0x06) = (short)(target_z - (int)cur_z);
        } else {
            short ax = SH(actor, 0x02);
            SH(actor, 0x02) = (ax > 0)
                ? (short)((int)ax + 0x32E0 - (int)max_x)
                : (short)((int)max_x + (int)ax - 0x32E0);
        }
    }

    if ((z >= 0 ? z : -z) < 0x3C0 && max_x >= 0x3555) {
        short ax = SH(actor, 0x02);
        SH(actor, 0x02) = (ax > 0)
            ? (short)((int)ax + 0x3554 - (int)max_x)
            : (short)((int)max_x + (int)ax - 0x3554);
    }
}
