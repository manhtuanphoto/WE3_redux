/*
 * func_800A0B60 - AI off-ball overlap / support-run avoidance & spacing adjustment
 *
 * Purpose:
 *   Prevents an AI teammate from crowding the nearest active chaser/teammate
 *   (Scratchpad+0x31E[dir]) around the target (outX, outZ) coordinate. Tracks a
 *   3-state avoidance machine in actor+0x3E3 (0 -> 1 -> 2 -> 0) and sets
 *   actor+0x3E2 = 0xB (or 0x11 when Scratchpad+0x2FC is nonzero), pushing
 *   (outX, outZ) radially 0x800 (2048 raw = 8.192 m) away from the teammate
 *   and clamping *outX to [-0x2E08, 0x2E08].
 *
 * Arguments:
 *   actor (unsigned char *) : pointer to 1000-byte player actor struct.
 *   outX  (signed short *)  : pointer to target X coordinate (R/W).
 *   outZ  (signed short *)  : pointer to target Z coordinate (R/W).
 *
 * Return value:
 *   void.
 *
 * Scratchpad (0x1F800000) reads:
 *   - 0x005 (u8, R): UNCLEAR active/designated player actor ID compared against actor+0x38D.
 *   - 0x2F4 (u8 *, R): pointer to ball actor (+0x02 s16 x).
 *   - 0x2FC (u8, R): pressing/event flag (nonzero overrides actor+0x3E2 = 0x11).
 *   - 0x2FF (u8[2], R): per-side tactical flag (indexed by actor+0x398).
 *   - 0x30B (u8, R): pressing/tactical mode (5 = early return).
 *   - 0x31E (u8[2], R): nearest player ID per attack direction (0 or 1).
 *
 * Actor fields read/written:
 *   - +0x02/+0x06 (s16, R): actor X/Z coordinates.
 *   - +0x38D (u8, R): actor ID (1..22).
 *   - +0x398 (u8, R): side (0/1).
 *   - +0x399 (u8, R): attack direction flag (0/1).
 *   - +0x3E2 (u8, W): AI movement/steering mode (set to 0xB or 0x11).
 *   - +0x3E3 (u8, R/W): avoidance sub-state (0, 1, or 2).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0xBC1 / -0xBC0 (3009 / -3008 raw = ~12.0 m): attacking-third ball X thresholds.
 *   - 0x50FFFF (~5.31M = ~2304^2 = (9.22 m)^2): max dist^2 from teammate to target.
 *   - 0x800 (2048 raw = 8.192 m): radial repulsion offset from teammate.
 *   - 0xFFFFF (~1.05M = ~1024^2 = (4.10 m)^2): min dist^2 to exit state 2.
 *   - 0x2E08 (11784 raw = 47.136 m): X-axis pitch clamp.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];

extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8001E694(unsigned int angle, int radius);
extern int func_8001E5A4(unsigned int angle, int radius);

void func_800A0B60(unsigned char *actor, signed short *outX, signed short *outZ)
{
    unsigned int side_flag;
    unsigned int push_flag = 0;
    unsigned int atk_dir;
    unsigned int ref_id;
    unsigned char *ref_actor;
    signed short dir_sign;
    signed short ref_x;
    signed short ref_z;
    int dx, dz;
    unsigned int dist_sq;
    unsigned int state;

    if (B(Scratchpad, 0x30B) == 5) {
        return;
    }

    side_flag = B(Scratchpad, 0x2FF + B(actor, 0x398));
    if (side_flag == 1) {
        unsigned int d = B(actor, 0x399);
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        if (d == 0 && S(ball, 0x2) >= 0xBC1) {
            push_flag = 1;
        } else if (d == side_flag && S(ball, 0x2) < -0xBC0) {
            push_flag = 1;
        }
    } else if (side_flag == 2) {
        push_flag = 1;
    }

    atk_dir = B(actor, 0x399);
    ref_id = B(Scratchpad, 0x31E + (atk_dir << 1));
    ref_actor = D_800FF748 + (ref_id % 24u) * 1000u;
    dir_sign = (atk_dir == 0) ? 1 : -1;

    ref_x = S(ref_actor, 0x2);
    ref_z = S(ref_actor, 0x6);
    dx = (int)ref_x - (int)*outX;
    dz = (int)ref_z - (int)*outZ;
    dist_sq = (unsigned int)(dx * dx + dz * dz);

    state = B(actor, 0x3E3);
    if (state == 0) {
        if (B(actor, 0x38D) == B(Scratchpad, 0x005) && dist_sq <= 0x50FFFFu) {
            unsigned int ang = func_8005B36C(ref_x, ref_z, S(actor, 0x2), S(actor, 0x6)) & 0xFFu;
            *outX = (signed short)(H(ref_actor, 0x2) + func_8001E694(ang, 0x800));
            *outZ = (signed short)(H(ref_actor, 0x6) + func_8001E5A4(ang, 0x800));
            B(actor, 0x3E2) = 0xB;
            B(actor, 0x3E3) = 1;
        }
    } else if (state == 1) {
        unsigned int ang = func_8005B36C(ref_x, ref_z, S(actor, 0x2), S(actor, 0x6)) & 0xFFu;
        *outX = (signed short)(H(ref_actor, 0x2) + func_8001E694(ang, 0x800));
        *outZ = (signed short)(H(ref_actor, 0x6) + func_8001E5A4(ang, 0x800));

        if ((push_flag & 0xFFu) != 0u) {
            if ((int)dir_sign * (int)S(actor, 0x2) < (int)dir_sign * (int)S(ref_actor, 0x2)) {
                *outX = (signed short)(H(ref_actor, 0x2) + func_8001E694(ang, 0x800) * (int)dir_sign);
            }
        }

        B(actor, 0x3E2) = 0xB;
        B(actor, 0x3E3) = B(actor, 0x3E3);
        if (B(actor, 0x38D) != B(Scratchpad, 0x005)) {
            B(actor, 0x3E2) = 0xB;
            B(actor, 0x3E3) = 0;
        }
    } else if (state == 2) {
        int dx2 = (int)ref_x - (int)S(actor, 0x2);
        int dz2 = (int)ref_z - (int)S(actor, 0x6);
        int dist_sq2 = dx2 * dx2 + dz2 * dz2;

        if (0xFFFFF < dist_sq2) {
            if (B(actor, 0x38D) != B(Scratchpad, 0x005)) {
                B(actor, 0x3E2) = 0xB;
                B(actor, 0x3E3) = 0;
            }
        } else {
            B(actor, 0x3E2) = 0xB;
            B(actor, 0x3E3) = 0;
        }

        if (B(Scratchpad, 0x2FC) != 0) {
            B(actor, 0x3E2) = 0x11;
            B(actor, 0x3E3) = 0;
        }
    }

    {
        signed short x_val = *outX;
        int abs_x = (x_val >= 0) ? x_val : -x_val;
        if (abs_x >= 0x2E09) {
            *outX = (x_val >= 0) ? 0x2E08 : -0x2E08;
        }
    }
}
