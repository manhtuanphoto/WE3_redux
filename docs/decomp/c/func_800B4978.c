/*
 * func_800B4978
 *
 * Purpose:
 *   Initializes a PsyQ GsCOORDINATE2-style 3D coordinate node at `coord`.
 *   Copies the 32-byte identity MATRIX D_800FB5B0 into `coord + 0x04..+0x23`,
 *   stores `super` at `coord + 0x48`, and clears `flg` at `coord + 0x00`.
 *   If `(unsigned int)super >= 2` (i.e., `super` is a valid parent pointer
 *   rather than 0 or 1/WORLD), links `*(super + 0x4C) = coord`.
 *
 * Arguments:
 *   super (unsigned int): parent GsCOORDINATE2 pointer (or 0 / 1 for root)
 *   coord (unsigned char *): pointer to GsCOORDINATE2 struct to initialize
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800FB5B0 (unsigned int[8]): 32-byte identity MATRIX
 */

#define W(p, o) (*(unsigned int *)((unsigned char *)(p) + (o)))

extern unsigned int D_800FB5B0[8];

void func_800B4978(unsigned int super, unsigned char *coord)
{
    int i;

    for (i = 0; i < 8; i++) {
        W(coord, 0x04 + i * 4) = D_800FB5B0[i];
    }

    W(coord, 0x48) = super;
    W(coord, 0x00) = 0;

    if (super >= 2U) {
        unsigned char *parent = (unsigned char *)W(coord, 0x48);
        W(parent, 0x4C) = (unsigned int)coord;
    }
}
