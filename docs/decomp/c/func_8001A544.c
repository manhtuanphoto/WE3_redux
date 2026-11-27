/*
 * func_8001A544
 *
 * Purpose:
 *   Builds and submits a player shadow/ground quad primitive when actor+0x39C
 *   is 0 or 1. Computes rotation/yaw (including height-interpolated orientation
 *   from func_8005C1CC and jtbl_800101E8 when +0x39C == 1, or fixed yaw 0xE00
 *   when +0x39C == 0), transforms 4 vertices from D_800FF748+0x7424+slot*128
 *   via func_80019E90 into D_800E8208, and links the primitive into the
 *   ordering table D_800F3560 at slot (4 << SCRATCH+0x00).
 *
 * Arguments:
 *   actor (unsigned char *): pointer to 1000-byte player actor struct
 *
 * Return value:
 *   void
 *
 * Scratchpad / Actor / Global fields read & written:
 *   SCRATCH 0x000 (u32, R): OT bit shift index (used as 4 << SCRATCH+0x00)
 *   SCRATCH 0x0F2 (s16, R/W): joint/limb Y height written by func_8005C1CC
 *   SCRATCH 0x124..0x128 (s16, W): local rotation angles (0, yaw, 0)
 *   SCRATCH 0x13C..0x144 (s32, W): local world translation (actor.x, 0, actor.z)
 *   SCRATCH 0x29D (u8,  R): active double-buffer index (0 or 1)
 *   D_800FF748 + 0x7424 + slot*0x80: 4 SVECTOR quad vertices
 *   D_800E8208 + buf*0x3AC0 + slot*0xA0 + 0x50: destination GPU primitive
 *   D_800F3560 + (buf << 14) + (4 << SCRATCH+0x00): OT head entry
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_800E8208[];
extern unsigned char D_800F3560[];
extern void *jtbl_800101E8[];

extern void func_8005C1CC(unsigned char *actor, unsigned int arg1);
extern void func_80019E90(unsigned char *verts, unsigned char *outPrim);

void func_8001A544(unsigned char *actor) {
    unsigned char buf_idx = B(SCRATCH, 0x29D);
    unsigned char *prim_base = D_800E8208 + (unsigned int)buf_idx * 0x3AC0u;
    int slot = (int)B(actor, 0x38D) - 1;
    unsigned char mode = B(actor, 0x39C);
    unsigned char *verts;
    unsigned char *prim_slot;
    unsigned char *prim;
    unsigned int ot_off;
    unsigned char *ot_entry;

    if (mode == 1) {
        signed short y_val;
        unsigned int yaw_byte;

        func_8005C1CC(actor, 1);
        y_val = S(SCRATCH, 0x0F2);
        SW(SCRATCH, 0x140) = 0;
        SW(SCRATCH, 0x13C) = S(actor, 0x02);
        SW(SCRATCH, 0x144) = S(actor, 0x06);

        if (y_val < -0xF0) {
            yaw_byte = 0xE0;
        } else {
            unsigned short act_id;
            unsigned int base_ang;
            signed short clamped_y;

            if (y_val >= -0x5F) {
                S(SCRATCH, 0x0F2) = 0;
            } else {
                S(SCRATCH, 0x0F2) = (signed short)(y_val + 0x60);
            }

            act_id = H(actor, 0x38E);
            if ((unsigned int)(act_id - 0x1Eu) < 0x5Au) {
                /*
                 * UNCLEAR: dispatches through jump table jtbl_800101E8[act_id - 0x1E]
                 * to one of 4 orientation formulas below; exact per-entry table
                 * mapping is not embedded in the .s file:
                 *   case A (.L8001A650): base_ang = 0x40 - actor[0x3AA];
                 *   case B (.L8001A65C): base_ang = (0x1C0 - actor[0x3AA]) + (actor[0x3AC] ? -0x40 : 0x40);
                 *   case C (.L8001A67C): base_ang = (0x1C0 - actor[0x3AA]) + (actor[0x3AC] ? 0x40 : -0x40);
                 *   case D (.L8001A69C): base_ang = 0xC0 - actor[0x3AA];
                 */
                (void)jtbl_800101E8;
                base_ang = 0xC0u - B(actor, 0x3AA);
            } else {
                base_ang = 0xC0u - B(actor, 0x3AA);
            }

            clamped_y = S(SCRATCH, 0x0F2);
            yaw_byte = (unsigned int)((((int)(base_ang & 0xFFu) * ((int)clamped_y + 0x90)) - ((int)clamped_y * 224)) / 144);
        }

        S(SCRATCH, 0x124) = 0;
        S(SCRATCH, 0x126) = (signed short)((yaw_byte & 0xFFu) << 4);
        S(SCRATCH, 0x128) = 0;
    } else if (mode == 0) {
        SW(SCRATCH, 0x140) = 0;
        SW(SCRATCH, 0x13C) = S(actor, 0x02);
        SW(SCRATCH, 0x144) = S(actor, 0x06);
        S(SCRATCH, 0x124) = 0;
        S(SCRATCH, 0x126) = 0x0E00;
        S(SCRATCH, 0x128) = 0;
    } else {
        return;
    }

    verts = D_800FF748 + 0x7424 + (slot << 7);
    prim_slot = prim_base + slot * 0xA0;
    prim = prim_slot + 0x50;

    func_80019E90(verts, prim);

    ot_off = ((unsigned int)B(SCRATCH, 0x29D) << 14) + (4u << W(SCRATCH, 0x00));
    ot_entry = D_800F3560 + ot_off;
    W(prim_slot, 0x50) = (W(prim_slot, 0x50) & 0xFF000000u) | (W(ot_entry, 0) & 0x00FFFFFFu);

    ot_off = ((unsigned int)B(SCRATCH, 0x29D) << 14) + (4u << W(SCRATCH, 0x00));
    ot_entry = D_800F3560 + ot_off;
    W(ot_entry, 0) = (W(ot_entry, 0) & 0xFF000000u) | ((unsigned int)prim & 0x00FFFFFFu);
}
