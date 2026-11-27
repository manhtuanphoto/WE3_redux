/*
 * func_8001D14C
 *
 * Purpose:
 *   Initializes an actor slot in D_800FF748 (24 x 1000-byte structs) with its
 *   type, actor ID, animation/model data, initial world position (x, 0, z),
 *   heading/yaw, and 1.12 fixed-point scale, while resetting velocity and
 *   action state fields.
 *
 * Arguments:
 *   actor_id (u8):  actor ID (slot index = (actor_id & 0xFF) % 24; 0 = ball, 23 = referee)
 *   arg1     (u8):  index into D_8013A000 model/asset table (used for actor_id 0 and 23)
 *   anim_id  (u8):  initial animation ID passed to func_8005BA30
 *   x        (s16): initial X coordinate
 *   z        (u16): initial Z coordinate (stack arg +0x10)
 *   heading  (u8):  initial heading byte 0..255 (stack arg +0x14)
 *   scale_x  (u32): 1.12 fixed-point X scale (stack arg +0x18)
 *   scale_y  (u32): 1.12 fixed-point Y scale (stack arg +0x1C)
 *   scale_z  (u32): 1.12 fixed-point Z scale (stack arg +0x20)
 *   type     (u16): actor type byte stored to actor+0x00 (stack arg +0x24)
 *
 * Return value:
 *   void
 *
 * Globals / Actor fields read & written:
 *   D_800FF748[(actor_id % 24) * 0x3E8]: actor struct initialized
 *     +0x00  (u8)   type / active flag
 *     +0x02  (s16)  x, +0x04 (s16) y = 0, +0x06 (s16) z
 *     +0x08/+0x0C/+0x10 (u32) velocities cleared to 0
 *     +0x20  (u32)  model/descriptor pointer
 *     +0x24/+0x28 (s16) rot x/z cleared to 0; +0x26 (s16) yaw = ((0x1C0 - heading) % 256) << 4
 *     +0x2C/+0x30/+0x34 (u32) scale x/y/z
 *     +0x38D (u8)   actor_id
 *     +0x396/+0x39C/+0x3A5/+0x3A7/+0x3A8/+0x3AC/+0x3D1/+0x3D2 cleared to 0
 *     +0x3A4/+0x3AA (u8) heading
 *     +0x3A6 (u8)   1 (or 12 for non-ball normal actors)
 *     +0x3D3 (u8)   1
 *   D_801C8004 (u32, R): descriptor used when type == 0x12
 *   D_801C8008 (u32, R): descriptor used when 0x13 <= type <= 0x1B
 *   D_8013A000[] (u32, R): descriptor table used when actor_id == 0 or 0x17
 *   D_8010C1D8 + 0x10 (u8, R): match minute / variant byte (mod 10 selects referee variant)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];
extern unsigned char D_8010C1D8[];
extern unsigned int  D_8013A000[];
extern unsigned int  D_801C8004;
extern unsigned int  D_801C8008;

extern void func_8001BC30(unsigned char *actor);
extern void func_8005BA30(unsigned char *actor, unsigned int anim_id, unsigned int arg2);

void func_8001D14C(
    unsigned int actor_id,
    unsigned int arg1,
    unsigned int anim_id,
    signed short x,
    unsigned short z,
    unsigned char heading,
    unsigned int scale_x,
    unsigned int scale_y,
    unsigned int scale_z,
    unsigned short type
) {
    unsigned int slot;
    unsigned char *actor;
    unsigned char actor_type;
    int yaw_raw;
    signed short yaw;

    slot = (actor_id & 0xFFu) % 24u;
    actor = D_800FF748 + slot * 0x3E8u;

    B(actor, 0x00) = (unsigned char)type;
    actor_type = B(actor, 0x00);
    B(actor, 0x38D) = (unsigned char)actor_id;
    B(actor, 0x3A5) = 0;

    if (actor_type == 0x12) {
        W(actor, 0x20) = D_801C8004;
        B(actor, 0x3A6) = 1;
        func_8001BC30(actor);
        func_8005BA30(actor, anim_id & 0xFFu, 0);
        B(actor, 0x39C) = 2;
        B(actor, 0x3D3) = 1;
        B(actor, 0x396) = 0xFF;
    } else if (actor_type >= 0x12 && actor_type < 0x1C) {
        W(actor, 0x20) = D_801C8008;
        B(actor, 0x3A6) = 1;
        func_8001BC30(actor);
        func_8005BA30(actor, anim_id & 0xFFu, 0);
        B(actor, 0x39C) = 2;
        B(actor, 0x3D3) = 1;
        B(actor, 0x396) = 0xFF;
    } else {
        unsigned int id_byte = actor_id & 0xFFu;
        unsigned int a6_val;

        if (id_byte == 0) {
            W(actor, 0x20) = D_8013A000[arg1 & 0xFFu];
        }
        if (id_byte == 0x17) {
            unsigned int rem = B(D_8010C1D8, 0x10) % 10u;
            W(actor, 0x20) = D_8013A000[(arg1 & 0xFFu) + rem];

            rem = B(D_8010C1D8, 0x10) % 10u;
            switch (rem) {
            case 0:
                scale_y = 0x11C7;
                scale_z = 0x0F0F;
                scale_x = 0x0F0F;
                break;
            case 1:
                scale_y = 0x1071;
                scale_z = 0x1000;
                scale_x = 0x1000;
                break;
            case 2:
                scale_y = 0x0F8E;
                scale_z = 0x0F33;
                scale_x = 0x0F33;
                break;
            case 3:
                scale_y = 0x1000;
                scale_z = 0x1000;
                scale_x = 0x1000;
                break;
            default:
                break;
            }
        }

        a6_val = actor_id & 0xFFu;
        B(actor, 0x3A6) = (a6_val == 0) ? 1 : 0x0C;
        func_8001BC30(actor);
        func_8005BA30(actor, anim_id & 0xFFu, 0);
        B(actor, 0x3D3) = 1;
    }

    yaw_raw = 0x1C0 - (int)heading;
    B(actor, 0x3A8) = heading;
    B(actor, 0x3AA) = heading;
    B(actor, 0x3A4) = heading;
    yaw = (signed short)((yaw_raw % 256) << 4);
    S(actor, 0x26) = yaw;

    B(actor, 0x3A8) = 0;
    B(actor, 0x3A7) = 0;
    S(actor, 0x02) = x;
    S(actor, 0x04) = 0;
    S(actor, 0x06) = (signed short)z;
    W(actor, 0x10) = 0;
    W(actor, 0x0C) = 0;
    W(actor, 0x08) = 0;
    S(actor, 0x28) = 0;
    S(actor, 0x24) = 0;
    S(actor, 0x26) = yaw;
    W(actor, 0x2C) = scale_x;
    W(actor, 0x30) = scale_y;
    W(actor, 0x34) = scale_z;
    B(actor, 0x3D2) = 0;
    B(actor, 0x39C) = 0;
    B(actor, 0x3D1) = 0;
    B(actor, 0x3AC) = 0;
    B(actor, 0x396) = 0;
}
