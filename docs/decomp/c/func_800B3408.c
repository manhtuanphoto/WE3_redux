/*
 * func_800B3408
 *
 * Purpose:
 *   Builds a 40-byte (0x28) PS1 GPU POLY_FT4-style primitive packet at `pkt`
 *   from template/OT state `tmpl` and four vertex structures (`v0`..`v3`),
 *   links it into the ordering table pointed to by `tmpl->ot` (at +0x14) with
 *   length 9 words (0x09000000), and advances the packet pointer by 0x28.
 *
 * UNCLEAR: Non-standard MIPS register-passing ABI (likely handwritten assembly).
 *   Inputs are read from $a0 (pkt), $a1 (tmpl), and $t0..$t3 (v0..v3);
 *   $a0 is incremented by 0x28 right before `jr $ra`.
 *
 * Packet layout written at pkt (0x28 bytes):
 *   +0x00: tag  = (*tmpl->ot) | 0x09000000; *tmpl->ot = (u32)pkt & 0x00FFFFFF
 *   +0x04: r0g0b0_code = tmpl->rgb_code (+0x10)
 *   +0x08: xy0  = v0->xy (+0x10)
 *   +0x0C: uv0_clut  = v0->uv (+0x08, u16) | (tmpl->clut (+0x0C, u16) << 16)
 *   +0x10: xy1  = v1->xy (+0x10)
 *   +0x14: uv1_tpage = v1->uv (+0x08, u16) | (tmpl->tpage (+0x0E, u16) << 16)
 *   +0x18: xy2  = v2->xy (+0x10)
 *   +0x1C: uv2  = v2->uv (+0x08, u16, zero-extended)
 *   +0x20: xy3  = v3->xy (+0x10)
 *   +0x24: uv3  = v3->uv (+0x08, u16, zero-extended)
 */

#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define PW(p, o) (*(unsigned int **)((p) + (o)))

unsigned char *func_800B3408(
    unsigned char *pkt,
    const unsigned char *tmpl,
    const unsigned char *v0,
    const unsigned char *v1,
    const unsigned char *v2,
    const unsigned char *v3)
{
    unsigned int *ot;
    unsigned int next;

    W(pkt, 0x0C) = (unsigned int)H(v0, 0x08) + ((unsigned int)H(tmpl, 0x0C) << 16);
    W(pkt, 0x14) = (unsigned int)H(v1, 0x08) + ((unsigned int)H(tmpl, 0x0E) << 16);
    W(pkt, 0x1C) = (unsigned int)H(v2, 0x08);
    W(pkt, 0x24) = (unsigned int)H(v3, 0x08);
    W(pkt, 0x04) = W(tmpl, 0x10);

    W(pkt, 0x08) = W(v0, 0x10);
    W(pkt, 0x10) = W(v1, 0x10);
    W(pkt, 0x18) = W(v2, 0x10);
    W(pkt, 0x20) = W(v3, 0x10);

    ot = PW(tmpl, 0x14);
    next = *ot;
    *ot = ((unsigned int)pkt << 8) >> 8;
    W(pkt, 0x00) = next | 0x09000000U;

    return pkt + 0x28;
}
