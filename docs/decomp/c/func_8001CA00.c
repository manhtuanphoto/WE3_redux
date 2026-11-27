/*
 * func_8001CA00 - Stadium / field prop & 3D mesh render dispatcher
 *
 * Purpose:
 *   Iterates over 18 renderable scene objects starting at D_800FF748+0x66D0
 *   (stride 0x90 bytes). For each active, visible object (type != 0 and
 *   +0x8D == 0), builds its GTE model-view matrix via func_800B6A78 /
 *   func_800B4A14 and dispatches by object type (1..31) through jump table
 *   jtbl_8001055C to submit quad/poly batches (func_800B2C28 / func_800B2728)
 *   and special meshes (func_80098E1C corner flags, func_8009A8FC goal nets).
 *
 * Arguments:
 *   None (void).
 *
 * Return value:
 *   void.
 *
 * Scratchpad (0x1F800000) reads/writes:
 *   - 0x000 (s32, R): OT shift offset subtracted from 0xE or 0xF.
 *   - 0x104 (MATRIX, R/W): combined GTE rotation/translation matrix.
 *   - 0x1E8 (u8, R): frustum/detail flag enabling func_800B2C28 fast path.
 *   - 0x1F0 (s32, R): camera/view X threshold (-0xCFF / +0xD00).
 *   - 0x1F4 (s32, R): camera/view Z/Y threshold (-0x83F).
 *   - 0x29D (u8, R): display buffer index (0 or 1), stride 20 into OT descriptors.
 *   - 0x2A4 (u8 *, R/W): running GPU packet/OT tail pointer.
 *
 * Globals read:
 *   - D_800FF748 (+0x66D0..+0x70DF, stride 0x90): 18 render object entries:
 *       +0x00 (u8): object type (1..31, 0 = inactive).
 *       +0x2C (u8[]): transform/rotation struct (first word cleared before func_800B6A78).
 *       +0x7C (u8 *): vertex table pointer.
 *       +0x80 (u32): extra render parameter.
 *       +0x84 (u8 *): primitive array pointer.
 *       +0x88 (u32): primitive count.
 *       +0x8D (u8): skip/hidden flag (nonzero skips).
 *   - D_800FF748+0x70F0: scratch packet workspace passed to func_800B2C28.
 *   - D_800FB580 (20-byte stride): ordering table descriptor array A.
 *   - D_800F1018 (20-byte stride): ordering table descriptor array B.
 *   - jtbl_8001055C: jump table for object types 1..31.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_800FB580[];
extern unsigned char D_800F1018[];

extern void func_800B6A78(unsigned char *src, unsigned char *dst_mat);
extern void func_800B4A14(unsigned char *mat);
extern unsigned int func_800B2C28(
    unsigned int prim_ptr, unsigned int vert_tbl, unsigned int arg2,
    unsigned int ot_tail, unsigned int count, unsigned int shift,
    unsigned char *ot_info, unsigned char *work_pkt
);
extern unsigned int func_800B2728(
    unsigned int prim_ptr, unsigned int vert_tbl, unsigned int arg2,
    unsigned int ot_tail, unsigned int count, unsigned int shift,
    unsigned char *ot_info
);
extern void func_80098E1C(unsigned char *obj);
extern void func_8009A8FC(unsigned char *obj);

void func_8001CA00(void)
{
    unsigned char *obj = D_800FF748 + 0x66D0;
    unsigned char *obj_hi = D_800FF748 + 0x6750; /* obj + 0x80 */
    int i;

    for (i = 0; i < 18; i++, obj += 0x90, obj_hi += 0x90) {
        unsigned int type_idx;

        if (B(obj, 0x00) == 0 || B(obj_hi, 0x0D) != 0) {
            continue;
        }

        W(obj_hi, -0x54) = 0; /* obj + 0x2C = 0 */
        func_800B6A78(obj + 0x2C, Scratchpad + 0x104);
        func_800B4A14(Scratchpad + 0x104);

        type_idx = B(obj, 0x00) - 1u;
        if (type_idx >= 0x1Fu) {
            continue;
        }

        switch (type_idx) {
        case 0:
        case 16:
        case 29:
        case 30: /* .L8001CAC0 */
            W(Scratchpad, 0x2A4) = func_800B2728(
                W(obj_hi, 0x04), W(obj_hi, -0x04), W(obj_hi, 0x00),
                W(Scratchpad, 0x2A4), W(obj_hi, 0x08), 0xE,
                D_800FB580 + B(Scratchpad, 0x29D) * 20
            );
            break;

        case 2:
        case 4: /* .L8001CAE8 */
            if (B(Scratchpad, 0x1E8) != 0 && SW(Scratchpad, 0x1F4) >= -0x83F && SW(Scratchpad, 0x1F0) >= -0xCFF) {
                W(Scratchpad, 0x2A4) = func_800B2C28(
                    W(obj_hi, 0x04), W(obj_hi, -0x04), W(obj_hi, 0x00),
                    W(Scratchpad, 0x2A4), W(obj_hi, 0x08), 0xE,
                    D_800FB580 + B(Scratchpad, 0x29D) * 20, D_800FF748 + 0x70F0
                );
            } else {
                W(Scratchpad, 0x2A4) = func_800B2728(
                    W(obj_hi, 0x04), W(obj_hi, -0x04), W(obj_hi, 0x00),
                    W(Scratchpad, 0x2A4), W(obj_hi, 0x08), 0xE,
                    D_800FB580 + B(Scratchpad, 0x29D) * 20
                );
            }
            break;

        case 1:
        case 3: /* .L8001CB94 */
            if (B(Scratchpad, 0x1E8) != 0 && SW(Scratchpad, 0x1F4) >= -0x83F && SW(Scratchpad, 0x1F0) < 0xD00) {
                W(Scratchpad, 0x2A4) = func_800B2C28(
                    W(obj_hi, 0x04), W(obj_hi, -0x04), W(obj_hi, 0x00),
                    W(Scratchpad, 0x2A4), W(obj_hi, 0x08), 0xE,
                    D_800FB580 + B(Scratchpad, 0x29D) * 20, D_800FF748 + 0x70F0
                );
            } else {
                W(Scratchpad, 0x2A4) = func_800B2728(
                    W(obj_hi, 0x04), W(obj_hi, -0x04), W(obj_hi, 0x00),
                    W(Scratchpad, 0x2A4), W(obj_hi, 0x08), 0xE,
                    D_800FB580 + B(Scratchpad, 0x29D) * 20
                );
            }
            break;

        case 10:
        case 11: /* .L8001CC40 */
            W(Scratchpad, 0x2A4) = func_800B2728(
                W(obj_hi, 0x04), W(obj_hi, -0x04), W(obj_hi, 0x00),
                W(Scratchpad, 0x2A4), W(obj_hi, 0x08), 0xE - SW(Scratchpad, 0x000),
                D_800F1018 + B(Scratchpad, 0x29D) * 20
            );
            func_80098E1C(obj);
            break;

        case 9:
        case 14: /* .L8001CC98 */
            W(Scratchpad, 0x2A4) = func_800B2728(
                W(obj_hi, 0x04), W(obj_hi, -0x04), W(obj_hi, 0x00),
                W(Scratchpad, 0x2A4), W(obj_hi, 0x08), 0xF - SW(Scratchpad, 0x000),
                D_800F1018 + B(Scratchpad, 0x29D) * 20
            );
            break;

        case 12:
        case 13: /* .L8001CCC4 */
            W(Scratchpad, 0x2A4) = func_800B2728(
                W(obj_hi, 0x04), W(obj_hi, -0x04), W(obj_hi, 0x00),
                W(Scratchpad, 0x2A4), W(obj_hi, 0x08), 0xE - SW(Scratchpad, 0x000),
                D_800F1018 + B(Scratchpad, 0x29D) * 20
            );
            func_8009A8FC(obj);
            break;

        case 5:
        case 6:
        case 7:
        case 8:
        case 27:
        case 28: /* .L8001CD1C */
            W(Scratchpad, 0x2A4) = func_800B2728(
                W(obj_hi, 0x04), W(obj_hi, -0x04), W(obj_hi, 0x00),
                W(Scratchpad, 0x2A4), W(obj_hi, 0x08), 0xE - SW(Scratchpad, 0x000),
                D_800F1018 + B(Scratchpad, 0x29D) * 20
            );
            break;

        case 15:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26: /* .L8001CD64 */
        default:
            break;
        }
    }
}
