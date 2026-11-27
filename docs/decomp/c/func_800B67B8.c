/*
 * func_800B67B8 - Evaluate and cache hierarchical transform chain into output buffer
 *
 * Purpose:
 *   Walks upward from `node` through parent pointers (Node +0x48) pushing nodes
 *   onto the D_8010C218 stack until reaching either a node already cached for the
 *   current stamp (D_8010CD50) or the root (parent == NULL). Copies the base
 *   32-byte transform into `out`, then walks back down the stack multiplying each
 *   child's local transform (+0x04) via func_800B4C14, caching the result (+0x24)
 *   and stamping (+0x00 = D_8010CD50).
 *
 *   Note: Identical to func_800B6A78 without the final func_800B4B94 call.
 *
 * Arguments:
 *   node (u8*) - pointer to scene-graph / bone node struct:
 *                +0x00 (u32) cache stamp (0 = uninitialized/dirty, D_8010CD50 = valid)
 *                +0x04..+0x20 (8 x u32) local 32-byte transform
 *                +0x24..+0x40 (8 x u32) cached world 32-byte transform
 *                +0x48 (u8*) parent node pointer
 *   out  (u8*) - pointer to 32-byte output transform buffer
 *
 * Return value:
 *   None (void).
 */

#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned char *D_8010C218[];
extern unsigned int D_8010CD50;

extern void func_800B4C14(unsigned char *dst, unsigned char *local);

static void copy_transform32(unsigned char *dst, int dst_off, unsigned char *src, int src_off)
{
    W(dst, dst_off + 0x00) = W(src, src_off + 0x00);
    W(dst, dst_off + 0x04) = W(src, src_off + 0x04);
    W(dst, dst_off + 0x08) = W(src, src_off + 0x08);
    W(dst, dst_off + 0x0C) = W(src, src_off + 0x0C);
    W(dst, dst_off + 0x10) = W(src, src_off + 0x10);
    W(dst, dst_off + 0x14) = W(src, src_off + 0x14);
    W(dst, dst_off + 0x18) = W(src, src_off + 0x18);
    W(dst, dst_off + 0x1C) = W(src, src_off + 0x1C);
}

void func_800B67B8(unsigned char *node, unsigned char *out)
{
    int depth = 0;
    int zero_idx = 100;

    while (1) {
        unsigned char *parent;
        unsigned int stamp;

        D_8010C218[depth] = node;
        parent = (unsigned char *)W(node, 0x48);
        stamp = W(node, 0x00);

        if (parent == 0) {
            if (stamp == D_8010CD50 || stamp == 0) {
                copy_transform32(node, 0x24, node, 0x04);
                copy_transform32(out, 0x00, node, 0x24);
                W(node, 0x00) = D_8010CD50;
                break;
            } else if (zero_idx == 100) {
                copy_transform32(out, 0x00, D_8010C218[0], 0x24);
                depth = 0;
                break;
            } else {
                depth = zero_idx + 1;
                copy_transform32(out, 0x00, D_8010C218[depth], 0x24);
                break;
            }
        } else {
            if (stamp == D_8010CD50) {
                copy_transform32(out, 0x00, node, 0x24);
                break;
            }
            if (stamp == 0) {
                zero_idx = depth;
            }
            node = parent;
            depth++;
        }
    }

    while (depth > 0) {
        unsigned char *cur = D_8010C218[depth - 1];
        func_800B4C14(out, cur + 0x04);
        depth--;
        copy_transform32(cur, 0x24, out, 0x00);
        W(cur, 0x00) = D_8010CD50;
    }
}
