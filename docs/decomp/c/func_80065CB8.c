/*
 * func_80065CB8
 * Purpose: Evaluates whether a forward/through pass from actor to target is viable.
 *          Checks pitch bounds, distance between actor and target, forward progression in
 *          attack direction, pass lane clearance via func_80065440, and ensures no active
 *          opponent is positioned directly in front of target within a +-32 (~45 deg) cone.
 * Arguments:
 *   actor  - pointer to 1000-byte passing player actor struct
 *   target - pointer to 1000-byte receiving teammate actor struct
 * Return value:
 *   1 if pass to target is clear/viable, 0 otherwise
 * Scratchpad reads/writes:
 *   none
 * Actor reads/writes:
 *   R: +0x02/+0x06 (x/z), +0x38D (actor id), +0x392 (team slot), +0x398 (side), +0x399 (attack dir)
 *   W: none
 * Globals:
 *   D_800FF748 (24-actor array, 1000-byte stride)
 *   D_8010C1D8 (u8 global disable/mode flag)
 *   D_8010C32E (u16 player stat field, 12-byte stride, 264 bytes/side)
 * Constants:
 *   0x1E01 (7681 raw = 30.724 m) max |target.z|; 0x23E3 (9187 raw = 36.748 m) max |actor.x|;
 *   0x1081 (4225 raw = 16.9 m) max |dz|; 0x27E3 (10211 raw = 40.844 m) max |target.x|;
 *   0x24E3 (9443 raw = 37.772 m) near-goal X; 0x601 (1537 raw = 6.148 m) near-goal |dz|;
 *   0x4000000 (dist <= 8192 raw = 32.768 m); 0x400000 (dist <= 2048 raw = 8.192 m);
 *   0x8FFFF (dist > 768 raw = 3.072 m); 0xB01 (2817 raw = 11.268 m);
 *   0x8B6 (2230 raw = 8.92 m); 0x841 (2113 raw = 8.452 m); 0x69A (1690 raw = 6.76 m);
 *   0x200 (512 raw = 2.048 m) lead X offset.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCH  ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010C1D8;
extern unsigned char D_8010C32E[];

extern int func_8001E6B8(int n);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_8005CE30(unsigned int actor_id);
extern unsigned int func_80065440(unsigned char *actor, unsigned int ang, unsigned int arg2, unsigned int dist_sq);

int func_80065CB8(unsigned char *actor, unsigned char *target) {
    int tz, az, ax, tx, dz, dx;
    int abs_tz, abs_ax, abs_dz, abs_tx, abs_az;
    unsigned int dist_sq;
    int dir_sign;
    int bonus;
    unsigned int ang;
    int stat_val;
    int lane_arg;
    unsigned int opp_start;
    unsigned char *opp;
    int lead_x;
    int i;
    int opp_x;
    unsigned int opp_dir;
    unsigned int diff;

    if (actor == target) {
        return 0;
    }
    if (D_8010C1D8 != 0) {
        return 0;
    }

    tz = S(target, 6);
    abs_tz = (tz >= 0) ? tz : -tz;
    if (abs_tz >= 0x1E01) { /* 30.724 m */
        return 0;
    }

    ax = S(actor, 2);
    abs_ax = (ax >= 0) ? ax : -ax;
    if (abs_ax >= 0x23E3) { /* 36.748 m */
        return 0;
    }

    az = S(actor, 6);
    dz = az - tz;
    abs_dz = (dz >= 0) ? dz : -dz;
    if (abs_dz >= 0x1081) { /* 16.9 m */
        return 0;
    }

    tx = S(target, 2);
    abs_tx = (tx >= 0) ? tx : -tx;
    if (abs_tx >= 0x27E3) { /* 40.844 m */
        return 0;
    }

    dx = ax - tx;
    dist_sq = (unsigned int)(dx * dx + dz * dz);
    if (dist_sq > 0x4000000) { /* > 32.768 m */
        return 0;
    }

    if (abs_tx >= 0x24E3) { /* >= 37.772 m */
        if (dist_sq > 0x400000) { /* > 8.192 m */
            return 0;
        }
        if (abs_dz >= 0x601) { /* >= 6.148 m */
            return 0;
        }
    }

    if (dist_sq <= 0x8FFFF) { /* <= 3.072 m */
        return 0;
    }

    dir_sign = (B(actor, 0x399) == 0) ? 1 : -1;
    abs_dz = (az - tz >= 0) ? (az - tz) : -(az - tz);
    if (abs_dz >= 0xB01) { /* >= 11.268 m */
        bonus = abs_dz / 8;
    } else {
        bonus = 0;
    }

    if (dir_sign * tx < dir_sign * ax + bonus) {
        return 0;
    }

    ang = func_8005B36C(ax, az, tx, tz);
    stat_val = H(D_8010C32E, B(actor, 0x398) * 0x108 + B(actor, 0x392) * 0xC) & 0xF;
    lane_arg = (4 - ((9 - stat_val) / 4)) & 0xFF;
    if ((func_80065440(actor, ang & 0xFF, lane_arg, dist_sq) & 0xFF) == 0) {
        return 0;
    }

    opp_start = func_8005CE30(B(actor, 0x38D)) & 0xFF;
    lead_x = 0;
    opp = D_800FF748 + opp_start * 0x3E8 + 0x3E8;

    abs_az = (S(actor, 6) >= 0) ? S(actor, 6) : -S(actor, 6);
    if (abs_az < 0x8B6) { /* < 8.92 m */
        stat_val = H(D_8010C32E, B(actor, 0x398) * 0x108 + B(actor, 0x392) * 0xC) & 0xF;
        if (func_8001E6B8(9 - stat_val) < 1) {
            lead_x = 0x200; /* 2.048 m */
        }
    }

    abs_dz = S(actor, 6) - S(target, 6);
    if (abs_dz < 0) {
        abs_dz = -abs_dz;
    }
    if (abs_dz >= 0xB01) { /* >= 11.268 m */
        lead_x = 0;
    } else if (abs_dz >= 0x841) { /* >= 8.452 m */
        lead_x = lead_x / 3;
    } else if (abs_dz >= 0x69A) { /* >= 6.76 m */
        lead_x = lead_x / 2;
    } else {
        abs_az = (S(actor, 6) >= 0) ? S(actor, 6) : -S(actor, 6);
        if (abs_az < 0x8B6) {
            stat_val = H(D_8010C32E, B(actor, 0x398) * 0x108 + B(actor, 0x392) * 0xC) & 0xF;
            if (func_8001E6B8(stat_val + 2) == 0) {
                lead_x = -0x200; /* -2.048 m */
            }
        }
    }

    lead_x = lead_x * (signed short)dir_sign;

    for (i = 0; (i & 0xFF) < 0xA; i++) {
        if (B(opp, 0) != 0) {
            opp_x = S(opp, 2);
            tx = S(target, 2);
            if (dir_sign * opp_x >= dir_sign * tx) {
                ang = func_8005B36C((signed short)(tx - lead_x), S(target, 6), opp_x, S(opp, 6));
                opp_dir = B(target, 0x399) << 7;
                diff = (opp_dir - ang) & 0xFF;
                if (diff < 0x81) {
                    if (diff < 0x20) {
                        return 0;
                    }
                } else {
                    if (((ang - opp_dir) & 0xFF) < 0x20) {
                        return 0;
                    }
                }
            }
        }
        opp += 0x3E8;
    }

    return 1;
}
