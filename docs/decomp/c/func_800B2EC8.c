/*
 * func_800B2EC8
 *
 * Purpose:
 *   PsyQ RotAverageNclip4 equivalent. Performs PS1 GTE perspective transform
 *   (`rtpt`) on 3D vertices `v0`, `v1`, `v2`, records GTE `FLAG` ($31) in `*flag`,
 *   and evaluates back-face culling via `nclip` (`MAC0` $24). If `mac0 > 0`
 *   (front-facing), stores screen coordinates `SXY0..SXY2` ($12..$14) to
 *   `*sxy0..*sxy2`, transforms `v3` via `rtps`, writes `SXY2` ($14) to `*sxy3`,
 *   `IR0` ($8) to `*p`, ORs the new GTE `FLAG` into `*flag`, computes average Z
 *   of all 4 vertices via `avsz4`, and stores `OTZ` ($7) to `*otz`.
 *   Returns the `nclip` cross-product `mac0` (<= 0 when culled).
 *
 * Arguments:
 *   v0, v1, v2, v3 (const void *): 3D SVECTOR inputs (8 bytes each: s16 x, y, z, pad)
 *   sxy0..sxy3     (unsigned int *): output packed screen XY words (s16 sx, s16 sy)
 *   p              (signed int *): output depth-cueing interpolation factor (GTE IR0)
 *   otz            (signed int *): output averaged Z value for ordering table (GTE OTZ)
 *   flag           (unsigned int *): output combined GTE status FLAG (ctrl reg $31)
 *
 * Return value:
 *   signed int: `nclip` result (GTE MAC0); > 0 if front-facing, <= 0 if culled
 */

signed int func_800B2EC8(
    const void *v0,
    const void *v1,
    const void *v2,
    const void *v3,
    unsigned int *sxy0,
    unsigned int *sxy1,
    unsigned int *sxy2,
    unsigned int *sxy3,
    signed int *p,
    signed int *otz,
    unsigned int *flag)
{
    unsigned int flag0 = 0;
    unsigned int flag1 = 0;
    signed int mac0 = 0;
    unsigned int gte_sxy0 = 0;
    unsigned int gte_sxy1 = 0;
    unsigned int gte_sxy2 = 0;
    unsigned int gte_sxy3 = 0;
    signed int gte_ir0 = 0;
    signed int gte_otz = 0;

    (void)v0;
    (void)v1;
    (void)v2;
    (void)v3;

    /* UNCLEAR: PS1 GTE COP2 sequence cannot be expressed in portable standard C
     * without COP2 intrinsics or inline assembly:
     *   lwc2 $0..$5 <- v0, v1, v2 (VXY0/VZ0, VXY1/VZ1, VXY2/VZ2)
     *   rtpt
     *   cfc2 flag0, $31 (GTE FLAG)
     */
    *flag = flag0;

    /* UNCLEAR: GTE nclip -> mfc2 mac0, $24 (MAC0) */
    if (mac0 > 0) {
        *sxy0 = gte_sxy0; /* swc2 $12 (SXY0) */
        *sxy1 = gte_sxy1; /* swc2 $13 (SXY1) */
        *sxy2 = gte_sxy2; /* swc2 $14 (SXY2) */

        /* UNCLEAR: PS1 GTE COP2 sequence:
         *   lwc2 $0..$1 <- v3 (VXY0/VZ0)
         *   rtps
         *   swc2 $14 -> *sxy3 (SXY2)
         *   swc2 $8  -> *p    (IR0)
         *   cfc2 flag1, $31   (FLAG)
         *   avsz4
         *   mfc2 gte_otz, $7  (OTZ)
         */
        *sxy3 = gte_sxy3;
        *p = gte_ir0;
        *flag = flag0 | flag1;
        *otz = gte_otz;
    }

    return mac0;
}
