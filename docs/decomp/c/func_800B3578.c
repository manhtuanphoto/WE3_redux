/*
 * func_800B3578
 *
 * Purpose:
 *   Inserts a PS1 GPU primitive packet into an ordering table (OT) at Z index
 *   `(u16)z - *(s32 *)(ot + 0x08)` (analogous to PsyQ GsSort* / AddPrim).
 *   If the computed OT index is negative, logs diagnostic D_8001504C via
 *   func_800AD698 before proceeding. Links `prim` at the head of `ot->org[idx]`,
 *   writes `(u8)len` into byte +0x03 of `prim` (PS1 DMA packet word-count tag),
 *   and clears byte +0x03 of the OT entry.
 *
 * Arguments:
 *   prim (unsigned char *): GPU primitive packet (word 0 is DMA next-tag)
 *   ot   (unsigned char *): ordering table descriptor (+0x04 = u32 *org, +0x08 = s32 z_offset)
 *   z    (unsigned short): primitive Z depth key
 *   len  (unsigned int): DMA packet payload length in 32-bit words (written to prim[3])
 *
 * Return value:
 *   None (void)
 *
 * Globals read:
 *   D_8001504C (char[], R): diagnostic message string for negative OT index
 */

#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern char D_8001504C[];
extern void func_800AD698(char *msg);

void func_800B3578(unsigned char *prim, unsigned char *ot, unsigned short z, unsigned int len)
{
    signed int idx = (signed int)(z & 0xFFFFU) - SW(ot, 0x08);
    unsigned char *entry;

    if (idx < 0) {
        func_800AD698(D_8001504C);
    }

    entry = (unsigned char *)(W(ot, 0x04) + ((unsigned int)idx << 2));

    /* Assembly also computes (prim + (((len & 0xFF) << 2) + 4)) into $v0, unused */
    W(prim, 0x00) = W(entry, 0x00);
    prim[0x03] = (unsigned char)len;
    W(entry, 0x00) = (unsigned int)prim;
    entry[0x03] = 0;
}
