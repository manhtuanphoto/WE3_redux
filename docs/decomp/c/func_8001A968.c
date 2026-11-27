/*
 * func_8001A968
 *
 * Purpose:
 *   Transforms and submits 4 GPU quad primitives (stride 0x28) for a player
 *   actor into the PS1 Ordering Table (D_800F3560). Computes local rotation/
 *   translation in Scratchpad+0x124/+0x13C, calls func_80019E90 to project the
 *   4-vertex quad from D_800FF748+0x7424 into D_800E8208, and links each
 *   primitive into the OT.
 *
 * Arguments:
 *   actor: (unsigned char*) pointer to player entity struct
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x000 (u32) OT Z-depth shift index (R)
 *     +0x0F2 (s16) elevation/tilt state adjusted when actor+0x39C == 1 (R/W)
 *     +0x124..+0x128 (3 x s16) local rotation angles passed to func_80019E90 (W)
 *     +0x13C..+0x144 (3 x s32) local translation (actor x, 0, z) (W)
 *     +0x29D (u8)  double-buffer frame index (0 or 1) (R)
 *   D_800E8208: primitive buffer (0x3AC0 per frame, 0xA0 per player, +0x50 offset)
 *   D_800F3560: Ordering Table base (0x4000 per frame)
 *   D_800FF748 + 0x7424: 4-quad vertex table (0x80 per player, 0x20 per quad)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800E8208[];
extern unsigned char D_800F3560[];
extern unsigned char D_800FF748[];

extern void func_8005C1CC(unsigned char *actor, int arg1);
extern void func_80019E90(unsigned char *verts, unsigned char *outPrim);

void func_8001A968(unsigned char *actor) {
    unsigned char buf_idx = B(Scratchpad, 0x29D);
    unsigned char *frame_prims = D_800E8208 + (buf_idx * 0x3AC0);
    int player_idx = B(actor, 0x38D) - 1;
    int part_angle = 0x20;
    int prim_off = 0;
    int part;

    for (part = 0; part < 4; part++) {
        unsigned char vis = B(actor, 0x39C);
        unsigned char *verts;
        unsigned char *out_prim;
        unsigned int ot_off;
        unsigned int *ot_entry;

        if (vis == 1) {
            signed short f2;
            unsigned short yaw;

            func_8005C1CC(actor, 1);
            f2 = S(Scratchpad, 0x0F2);

            SW(Scratchpad, 0x13C) = S(actor, 0x02);
            SW(Scratchpad, 0x140) = 0;
            SW(Scratchpad, 0x144) = S(actor, 0x06);

            if (f2 < -0xF0) {
                yaw = (unsigned short)part_angle;
            } else {
                int adj_f2;
                unsigned int anim_off;
                unsigned int base_ang = 0;

                if (f2 < -0x5F) {
                    S(Scratchpad, 0x0F2) = (signed short)(f2 + 0x60);
                } else {
                    S(Scratchpad, 0x0F2) = 0;
                }

                anim_off = (unsigned int)(H(actor, 0x38E) - 0x1E);
                if (anim_off < 0x5A) {
                    /* UNCLEAR: jtbl_80010350 maps actor+0x38E in 0x1E..0x77 to one of:
                     *   case .L8001AA7C: base_ang = 0x40 - B(actor, 0x3AA);
                     *   case .L8001AA88: base_ang = 0x1C0 - B(actor, 0x3AA) + (B(actor, 0x3AC) ? -0x40 : 0x40);
                     *   case .L8001AAA8: base_ang = 0x1C0 - B(actor, 0x3AA) + (B(actor, 0x3AC) ? 0x40 : -0x40);
                     *   default (.L8001AAC8): base_ang = 0xC0 - B(actor, 0x3AA);
                     */
                    base_ang = 0xC0U - B(actor, 0x3AA);
                } else {
                    base_ang = 0xC0U - B(actor, 0x3AA);
                }

                adj_f2 = S(Scratchpad, 0x0F2);
                yaw = (unsigned short)((((int)(base_ang & 0xFF) * (adj_f2 + 0x90)) - (part_angle * adj_f2)) / 0x90);
            }

            S(Scratchpad, 0x124) = 0;
            S(Scratchpad, 0x126) = (signed short)((yaw & 0xFF) << 4);
            S(Scratchpad, 0x128) = 0;
        } else if (vis == 0) {
            SW(Scratchpad, 0x13C) = S(actor, 0x02);
            SW(Scratchpad, 0x140) = 0;
            SW(Scratchpad, 0x144) = S(actor, 0x06);
            S(Scratchpad, 0x124) = 0;
            S(Scratchpad, 0x126) = (signed short)(part_angle << 4);
            S(Scratchpad, 0x128) = 0;
        } else {
            break;
        }

        verts = D_800FF748 + 0x7424 + (player_idx << 7) + (part << 5);
        out_prim = frame_prims + (player_idx * 0xA0) + 0x50 + prim_off;
        func_80019E90(verts, out_prim);

        part_angle += 0x40;

        ot_off = (B(Scratchpad, 0x29D) << 14) + (4U << W(Scratchpad, 0x000));
        ot_entry = (unsigned int *)(D_800F3560 + ot_off);

        W(out_prim, 0x00) = (W(out_prim, 0x00) & 0xFF000000U) | (*ot_entry & 0x00FFFFFFU);
        *ot_entry = (*ot_entry & 0xFF000000U) | ((unsigned int)out_prim & 0x00FFFFFFU);

        prim_off += 0x28;
    }
}
