/*
 * func_8019CD0C (select overlay)
 *
 * Purpose:
 *   Updates and submits up to 4 carousel UI slot primitives (plus their backing
 *   frame sprite 0x3022) via func_80193FB4 during menu carousel rotation.
 *   Uses D_801D8C60 as the rotation phase/offset (0..31) to index X-offset
 *   (D_80189FB8) and scale/brightness (D_80189F78) lookup tables. When
 *   Scratchpad+0x2E1 == 1 or (Scratchpad+0x2E1 == 2 && (D_80108667 & 0xF) == 0),
 *   slot 2 (D_801D8C66, obj 0x18/0x19) is omitted and the adjacent slot offset
 *   is doubled to bridge the gap.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_801D8C60 (s8/u8): carousel rotation step/phase
 *   D_801D8C64..D_801D8C67 (u8): item IDs for the 4 carousel slots
 *   D_80108667 (u8): mode/rules bitfield (low nibble checked)
 *   D_80189F78 (u16[32]): scale table (also >>= 5 for RGB brightness)
 *   D_80189FB8 (s8[32]): signed X-position offset table
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x2E1 (u8): match/competition mode (1 or 2 suppresses slot 2 when rules == 0)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern signed char D_801D8C60;
extern unsigned char D_801D8C64;
extern unsigned char D_801D8C65;
extern unsigned char D_801D8C66;
extern unsigned char D_801D8C67;
extern unsigned char D_80108667;

extern unsigned short D_80189F78[32];
extern signed char D_80189FB8[32];

extern unsigned short func_8019E454(unsigned int item_id, unsigned int a1);
extern unsigned short func_8019E494(unsigned int angle_idx);
extern void func_80193FB4(
    unsigned int obj_id, unsigned int sprite_id, unsigned int a2, unsigned int a3,
    int x, int y, int z, unsigned int scale_x, unsigned int scale_y,
    int off_x, int off_y, unsigned int r, unsigned int g, unsigned int b,
    unsigned int a14, unsigned int prio, unsigned int flag);

static void draw_carousel_slot(unsigned int obj_id, unsigned char item_id, unsigned int angle_idx)
{
    unsigned int sprite_id = func_8019E454(item_id, 1);
    unsigned int scale = D_80189F78[angle_idx];
    unsigned int shade = scale >> 5;
    int x = (int)D_80189FB8[angle_idx];
    unsigned int prio;

    prio = func_8019E494(angle_idx);
    func_80193FB4(obj_id, sprite_id, 0, 0, x, 0x18, 0, scale, scale,
                 -0x58, -0x21, shade, shade, shade, 0, prio, 1);

    prio = func_8019E494(angle_idx);
    func_80193FB4(obj_id + 1U, 0x3022, 0, 0, x, 0x18, 0, scale, scale,
                 -0x58, -0x21, shade, shade, shade, 0, prio, 1);
}

void func_8019CD0C(void)
{
    signed char step = D_801D8C60;
    unsigned int idx0 = (unsigned int)D_801D8C60 & 0x1FU;
    unsigned int idx1 = (unsigned int)((int)step + 8) & 0x1FU;
    unsigned int idx2 = (unsigned int)((int)step + 0x10) & 0x1FU;
    unsigned int idx3 = (unsigned int)((int)step + 0x18) & 0x1FU;
    unsigned char mode = B(Scratchpad, 0x2E1);

    if ((mode == 2 && (D_80108667 & 0xF) == 0) || mode == 1) {
        signed char s = D_801D8C60;
        if (s < 0) {
            idx3 = (unsigned int)(((int)s << 1) + 0x18) & 0x1FU;
        } else {
            idx1 = (unsigned int)(((int)s << 1) + 8) & 0x1FU;
        }
    }

    draw_carousel_slot(0x14, D_801D8C64, idx0);
    draw_carousel_slot(0x16, D_801D8C65, idx1);

    mode = B(Scratchpad, 0x2E1);
    if (!((mode == 2 && (D_80108667 & 0xF) == 0) || mode == 1)) {
        draw_carousel_slot(0x18, D_801D8C66, idx2);
    }

    draw_carousel_slot(0x1A, D_801D8C67, idx3);
}
