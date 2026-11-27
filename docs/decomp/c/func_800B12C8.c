/*
 * func_800B12C8
 *
 * Purpose:
 *   PsyQ GTE 3x3 matrix * SVECTOR -> VECTOR multiplication (ApplyMatrix).
 *   Loads the 3x3 rotation matrix from m (5 words = 9 signed shorts + pad)
 *   into GTE control registers R11R12..R33 ($0..$4), loads the 16-bit input
 *   vector v0 into GTE VXY0/VZ0 ($0/$1), executes GTE MVMVA(sf=1, mx=0, v=0,
 *   cv=3, lm=0), and stores the 32-bit MAC1/MAC2/MAC3 results (shifted >> 12)
 *   into out->vx, out->vy, out->vz.
 *
 * Arguments:
 *   m   (const short *): 3x3 fixed-point matrix (1.3.12 format, 9 shorts)
 *   v0  (const short *): input SVECTOR {vx, vy, vz}
 *   out (int *):         output VECTOR {vx, vy, vz}
 *
 * Return value:
 *   int *: returns out.
 */

int *func_800B12C8(const short *m, const short *v0, int *out)
{
    /* GTE MVMVA(sf=1, mx=RT, v=V0, cv=NONE, lm=0): 12-bit fixed-point multiply */
    out[0] = ((int)m[0] * v0[0] + (int)m[1] * v0[1] + (int)m[2] * v0[2]) >> 12;
    out[1] = ((int)m[3] * v0[0] + (int)m[4] * v0[1] + (int)m[5] * v0[2]) >> 12;
    out[2] = ((int)m[6] * v0[0] + (int)m[7] * v0[1] + (int)m[8] * v0[2]) >> 12;
    return out;
}
