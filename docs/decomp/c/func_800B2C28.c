/*
 * func_800B2C28
 *
 * Purpose:
 *   Processes a list of 32-byte textured quad descriptors, transforms/projects
 *   their 4 indexed vertices via func_800B2EC8 and func_800B2EA8, builds a
 *   POLY_FT4-style GPU primitive packet (command 0x2C808080) with CLUT/tpage/UVs
 *   and OT pointer, and submits it via func_800B2F80.
 *
 * Arguments:
 *   polys (u8 *): array of 32-byte quad descriptors (base + 0x10 walked per iteration):
 *     +0x03 (u8) command flags (ORed with D_8010C1B8 << 1 -> packet+0x13)
 *     +0x04/+0x08/+0x0C/+0x10 (u32) UV/CLUT/tpage packed words:
 *       +0x04 (u32) uv0, +0x06 (u16) clut, +0x08 (u32) uv1, +0x0A (u16) tpage,
 *       +0x0C (u32) uv2, +0x10 (u32) uv3
 *     +0x16/+0x18/+0x1A/+0x1C (u16) 4 vertex indices into verts[]
 *   verts (u8 *): array of 8-byte SVECTOR vertices (unaligned copy supported)
 *   unused_a2 (u32): unused in function body
 *   acc (u32): running accumulator/pointer threaded through func_800B2F80 and returned
 *   count (u32, stack +0x80): number of quads to process
 *   ot_shift (u32, stack +0x84): right shift applied to (sz - ot_zbase) for OT bucket
 *   ot_info (u32 *, stack +0x88): [1]=OT base pointer, [2]=OT Z base
 *   pkt (u8 *, stack +0x8C): scratch packet buffer (0x100 bytes; vertex slots at +0x18, +0x30, +0x48, +0x60, +0x78)
 *
 * Return value:
 *   Updated acc (u32) returned from the final func_800B2F80 call.
 *
 * Global fields read:
 *   D_8010C1B8 (u8): semi-transparency / render flag shifted << 1 into packet+0x13
 *
 * Global fields written:
 *   None
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define H(p, o) (*(unsigned short *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned char D_8010C1B8;

extern int func_800B2EC8(
    unsigned char *v0, unsigned char *v1, unsigned char *v2, unsigned char *v3,
    unsigned char *out0, unsigned char *out1, unsigned char *out2, unsigned char *out3,
    unsigned int *p0, unsigned int *p1, unsigned int *p2
);
extern void func_800B2EA8(unsigned char *s0, unsigned char *s1, unsigned char *s2, unsigned char *s3);
extern unsigned int func_800B2F80(unsigned int acc, unsigned char *pkt, unsigned int a2, unsigned char *scratch);

static void copy_u64_unaligned(unsigned char *dst, const unsigned char *src)
{
    int i;
    for (i = 0; i < 8; i++) {
        dst[i] = src[i];
    }
}

unsigned int func_800B2C28(
    unsigned char *polys,
    const unsigned char *verts,
    unsigned int unused_a2,
    unsigned int acc,
    unsigned int count,
    unsigned int ot_shift,
    const unsigned int *ot_info,
    unsigned char *pkt
)
{
    unsigned char *v0 = pkt + 0x18;
    unsigned char *v1 = pkt + 0x30;
    unsigned char *v2 = pkt + 0x48;
    unsigned char *v3 = pkt + 0x60;
    unsigned char *scratch = pkt + 0x78;
    unsigned int i;
    unsigned char *cur;

    (void)unused_a2;

    W(pkt, 0xF0) = (unsigned int)v0;
    W(pkt, 0xF4) = (unsigned int)v1;
    W(pkt, 0xF8) = (unsigned int)v2;
    W(pkt, 0xFC) = (unsigned int)v3;

    if (count == 0) {
        return acc;
    }

    cur = polys + 0x10;
    for (i = 0; i < count; i++, cur += 0x20) {
        unsigned int sp30;
        unsigned int sp34;
        unsigned int sp38;
        int vis;

        copy_u64_unaligned(v0, verts + ((unsigned int)H(cur, 0x6) << 3));
        copy_u64_unaligned(v1, verts + ((unsigned int)H(cur, 0x8) << 3));
        copy_u64_unaligned(v2, verts + ((unsigned int)H(cur, 0xA) << 3));
        copy_u64_unaligned(v3, verts + ((unsigned int)H(cur, 0xC) << 3));

        vis = func_800B2EC8(
            v0, v1, v2, v3,
            v0 + 0x10, v1 + 0x10, v2 + 0x10, v3 + 0x10,
            &sp30, &sp34, &sp38
        );

        if (vis > 0) {
            unsigned int ot_idx;

            func_800B2EA8(v0 + 0x14, v1 + 0x14, v2 + 0x14, v3 + 0x14);

            ot_idx = ((sp34 & 0xFFFF) - ot_info[2]) >> ot_shift;
            W(pkt, 0x10) = 0x2C808080;
            W(pkt, 0x14) = ot_info[1] + (ot_idx << 2);
            B(pkt, 0x13) = B(cur, -0xD) | (unsigned char)(D_8010C1B8 << 1);
            H(pkt, 0xC) = H(cur, -0xA);
            H(pkt, 0xE) = H(cur, -0x6);
            W(v0, 0x8) = W(cur, -0xC);
            W(v1, 0x8) = W(cur, -0x8);
            W(v2, 0x8) = W(cur, -0x4);
            W(v3, 0x8) = W(cur, 0x0);

            acc = func_800B2F80(acc, pkt, 0, scratch);
        }
    }

    return acc;
}
