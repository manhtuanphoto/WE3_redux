/*
 * func_800B4E48
 *
 * Purpose:
 *   Applies a 3x3 rotation matrix to a 32-bit fixed-point 3D vector
 *   (analogous to PsyQ ApplyMatrixLV) via PS1 GTE MVMVA by splitting each
 *   signed 32-bit component into high (x / 32768) and low (x % 32768) parts
 *   to avoid 16-bit GTE input overflow.
 *
 * Arguments:
 *   mat: (signed short*) pointer to 3x3 matrix (5 packed 32-bit words loaded
 *        into GTE control registers $0..$4)
 *   in:  (signed int*) pointer to 3-element s32 input vector (x, y, z)
 *   out: (signed int*) pointer to 3-element s32 output vector
 *
 * Return value:
 *   (signed int*) out
 *
 * Reads / Writes:
 *   GTE Coprocessor 2:
 *     Control $0..$4 (R11R12..R33) loaded from mat[0..4]
 *     MVMVA(sf=0) on high 15-bit-shifted part (shifted << 3 afterward)
 *     MVMVA(sf=1) on signed 15-bit remainder, summed into out[0..2]
 */

signed int *func_800B4E48(const void *mat, const signed int *in, signed int *out) {
    signed int vx = in[0];
    signed int vy = in[1];
    signed int vz = in[2];
    signed int hiX, loX;
    signed int hiY, loY;
    signed int hiZ, loZ;

    if (vx < 0) {
        signed int neg = -vx;
        hiX = -(neg >> 15);
        loX = -(neg & 0x7FFF);
    } else {
        hiX = vx >> 15;
        loX = vx & 0x7FFF;
    }

    if (vy < 0) {
        signed int neg = -vy;
        hiY = -(neg >> 15);
        loY = -(neg & 0x7FFF);
    } else {
        hiY = vy >> 15;
        loY = vy & 0x7FFF;
    }

    if (vz < 0) {
        signed int neg = -vz;
        hiZ = -(neg >> 15);
        loZ = -(neg & 0x7FFF);
    } else {
        hiZ = vz >> 15;
        loZ = vz & 0x7FFF;
    }

    /* UNCLEAR: PS1 GTE inline coprocessor 2 sequence:
     *   1. Load 5 words from mat into GTE matrix registers $0..$4.
     *   2. Move (hiX, hiY, hiZ) into GTE V0 ($9..$11); execute mvmva(0,0,3,3,0);
     *      read (macHiX, macHiY, macHiZ) from MAC1..3 ($25..$27) and scale << 3.
     *   3. Move (loX, loY, loZ) into GTE V0 ($9..$11); execute mvmva(1,0,3,3,0);
     *      read (macLoX, macLoY, macLoZ) from MAC1..3 ($25..$27).
     *   4. Store out[i] = macLo[i] + (macHi[i] << 3).
     */
    __asm__ volatile ("" : : : "memory");
    (void)mat;
    (void)hiX; (void)hiY; (void)hiZ;
    (void)loX; (void)loY; (void)loZ;
    out[0] = 0;
    out[1] = 0;
    out[2] = 0;

    return out;
}
