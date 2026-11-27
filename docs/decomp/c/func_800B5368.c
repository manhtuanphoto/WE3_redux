/*
 * func_800B5368
 *
 * Purpose:
 *   Relocates relative offsets inside a PS1 TMD 3D model header into absolute
 *   RAM pointers (PsyQ GsMapModelingData-style).
 *   Checks bit 0 of `tmd[0]` (FIXP flag); if already set, returns immediately.
 *   Otherwise sets bit 0, reads object count `nobj = (signed int)tmd[1]`, and
 *   for each of the `nobj` 28-byte (0x1C) TMD object entries starting at `tmd + 2`
 *   (`obj_base`), adds `obj_base` to the vertex (+0x00), normal (+0x08), and
 *   primitive (+0x10) offset fields.
 *
 * Arguments:
 *   tmd (unsigned int *): pointer to TMD data in RAM
 *     +0x00 (u32): TMD ID / flags (bit 0 = already relocated)
 *     +0x04 (s32): number of objects (nobj)
 *     +0x08..:     array of nobj 0x1C-byte object headers
 *
 * Return value:
 *   None (void)
 */

#define W(p, o) (*(unsigned int *)((unsigned char *)(p) + (o)))

void func_800B5368(unsigned int *tmd)
{
    unsigned int flags = tmd[0];
    signed int nobj;
    unsigned char *obj_base;
    unsigned char *cur;
    signed int i;

    if ((flags & 0x1U) != 0U) {
        return;
    }

    tmd[0] = flags | 0x1U;
    nobj = (signed int)tmd[1];
    if (nobj <= 0) {
        return;
    }

    obj_base = (unsigned char *)(tmd + 2);
    cur = obj_base;

    for (i = 0; i < nobj; i++) {
        W(cur, 0x00) += (unsigned int)obj_base; /* vert_top */
        W(cur, 0x10) += (unsigned int)obj_base; /* primitive_top */
        W(cur, 0x08) += (unsigned int)obj_base; /* normal_top */
        cur += 0x1C;
    }
}
