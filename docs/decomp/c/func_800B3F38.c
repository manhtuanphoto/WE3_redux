/*
 * func_800B3F38
 *
 * Purpose:
 *   PS1 GTE 4-vertex perspective transformation (PsyQ RotTransPers4).
 *   Loads 3D SVECTOR vertices v0, v1, v2 into GTE V0..V2 and executes RTPT
 *   (Rotate, Translate, Perspective Triple), writes screen coordinates SXY0..SXY2
 *   to *xy0..*xy2, and captures GTE FLAG ($31). Then loads v3 into GTE V0,
 *   executes RTPS (Rotate, Translate, Perspective Single), writes SXY2 to *xy3,
 *   depth-cue factor IR0 ($8) to *out_p, combined FLAGs to *out_flag, and
 *   returns SZ3 ($19) >> 2 as the ordering-table Z depth.
 *
 * Arguments:
 *   v0..v3   (signed short *): 3D SVECTOR inputs (x, y, z at +0x00, +0x02, +0x04)
 *   xy0..xy3 (unsigned int *): packed screen-space (s16 x, s16 y) outputs
 *   out_p    (unsigned int *): GTE IR0 ($8) interpolation/fog output
 *   out_flag (unsigned int *): bitwise OR of GTE FLAG ($31) from RTPT and RTPS
 *
 * Return value:
 *   signed int: SZ3 >> 2 (OT Z coordinate of v3)
 *
 * Notes:
 *   UNCLEAR: Relies on implicit GTE cop2 control registers (RT, TR, OFX, OFY,
 *   H, DQA, DQB) configured by the caller prior to invocation.
 */

extern void gte_ldv3(const signed short *v0, const signed short *v1, const signed short *v2);
extern void gte_rtpt(void);
extern void gte_stsxy3(unsigned int *xy0, unsigned int *xy1, unsigned int *xy2);
extern unsigned int gte_stflg(void);
extern void gte_ldv0(const signed short *v3);
extern void gte_rtps(void);
extern void gte_stsxy2(unsigned int *xy3);
extern void gte_stopz_ir0(unsigned int *out_p);
extern signed int gte_stsz(void);

signed int func_800B3F38(
    signed short *v0,
    signed short *v1,
    signed short *v2,
    signed short *v3,
    unsigned int *xy0,
    unsigned int *xy1,
    unsigned int *xy2,
    unsigned int *xy3,
    unsigned int *out_p,
    unsigned int *out_flag)
{
    unsigned int flag_rtpt;
    unsigned int flag_rtps;
    signed int sz3;

    /* lwc2 $0..$5 from v0..v2; rtpt; swc2 $12..$14 -> xy0..xy2 */
    gte_ldv3(v0, v1, v2);
    gte_rtpt();
    gte_stsxy3(xy0, xy1, xy2);
    flag_rtpt = gte_stflg();

    /* lwc2 $0..$1 from v3; rtps; swc2 $14 -> xy3, swc2 $8 -> out_p */
    gte_ldv0(v3);
    gte_rtps();
    gte_stsxy2(xy3);
    gte_stopz_ir0(out_p);
    flag_rtps = gte_stflg();
    sz3 = gte_stsz();

    *out_flag = flag_rtpt | flag_rtps;
    return sz3 >> 2;
}
