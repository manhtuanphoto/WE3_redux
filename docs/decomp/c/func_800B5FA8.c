/*
 * func_800B5FA8 - 3D camera / look-at view matrix builder
 *
 * Purpose:
 *   Constructs a 32-byte PS1 GTE rotation+translation view matrix in D_800FF6B0
 *   from a camera descriptor struct, starting from identity/base matrix D_800FF708,
 *   applying roll (+0x18), look-at pitch/yaw from func_800B6378 eye/target points,
 *   camera translation (-pos at +0x00..+0x08), and an optional extra transform
 *   (+0x1C), then copies the final matrix to D_800EF8F8.
 *
 * Arguments:
 *   cam (unsigned char *): camera/transform struct:
 *     +0x00..+0x08 (s32[3], R): camera world position (X, Y, Z).
 *     +0x18 (s32, R): roll/base Y-axis rotation angle.
 *     +0x1C (s32, R): optional secondary rotation/offset parameter (nonzero applies extra transform).
 *
 * Return value:
 *   int: 0 on success, 1 if the eye-to-target 3D distance is 0.
 *
 * Globals read/written:
 *   - D_800FF708 (32 bytes, R): base/identity matrix copied into D_800FF6B0.
 *   - D_800FF6B0 (32 bytes, R/W): working camera matrix (3x3 s16 + pad + s32[3] translation at +0x14).
 *   - D_800EF8F8 (32 bytes, W): final output view matrix copied from D_800FF6B0.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF6B0[];
extern unsigned char D_800FF708[];
extern unsigned char D_800EF8F8[];

extern void func_800B6588(unsigned char *mat, signed int angle);
extern void func_800B6378(unsigned char *cam, unsigned char *out_pts);
extern unsigned int func_800B1128(int val);
extern void func_800B5E48(unsigned char *mat, signed short sin_val, signed short cos_val, int axis);
extern void func_800B50F8(unsigned char *dst, unsigned char *src);
extern void func_800B4E48(unsigned char *mat, unsigned char *vec_in, unsigned char *vec_out);
extern void func_800B67B8(signed int param, unsigned char *mat);
extern void func_800B6548(unsigned char *src_mat, unsigned char *dst_mat);
extern void func_800B4B94(unsigned char *m0, unsigned char *m1);

static void copy32(unsigned char *dst, unsigned char *src)
{
    W(dst, 0x00) = W(src, 0x00);
    W(dst, 0x04) = W(src, 0x04);
    W(dst, 0x08) = W(src, 0x08);
    W(dst, 0x0C) = W(src, 0x0C);
    W(dst, 0x10) = W(src, 0x10);
    W(dst, 0x14) = W(src, 0x14);
    W(dst, 0x18) = W(src, 0x18);
    W(dst, 0x1C) = W(src, 0x1C);
}

int func_800B5FA8(unsigned char *cam)
{
    signed int pts[6];      /* sp+0x10..0x24: [eyeX, eyeY, eyeZ, targetX, targetY, targetZ] */
    unsigned char rot_mat[32]; /* sp+0x30..0x4C (note sp+0x44..0x4C overlaps unused/padding area in stack) */
    unsigned char tmp_mat[32]; /* sp+0x50..0x6C */
    signed int neg_pos[3];  /* sp+0x90..0x98 */
    int dx, dy, dz;
    int dist3d;
    int dist_xz;
    int sin_pitch;
    int cos_pitch;

    copy32(D_800FF6B0, D_800FF708);
    func_800B6588(D_800FF6B0, -SW(cam, 0x18));
    func_800B6378(cam, (unsigned char *)pts);

    dx = pts[3] - pts[0];
    dy = pts[4] - pts[1];
    dz = pts[5] - pts[2];
    dist3d = (int)func_800B1128(dx * dx + dy * dy + dz * dz);
    if (dist3d == 0) {
        return 1;
    }

    sin_pitch = -(((pts[1] - pts[4]) << 12) / dist3d);
    dist_xz = (int)func_800B1128(dx * dx + dz * dz);
    cos_pitch = (dist_xz << 12) / dist3d;

    /* 'x' = 0x78: X-axis (pitch) rotation */
    func_800B5E48(rot_mat, (signed short)sin_pitch, (signed short)cos_pitch, 0x78);
    func_800B50F8(D_800FF6B0, rot_mat);

    if (dist_xz != 0) {
        int sin_yaw = -(((pts[3] - pts[0]) << 12) / dist_xz);
        int cos_yaw = ((pts[5] - pts[2]) << 12) / dist_xz;
        /* 'y' = 0x79: Y-axis (yaw) rotation */
        func_800B5E48(rot_mat, (signed short)sin_yaw, (signed short)cos_yaw, 0x79);
        func_800B50F8(D_800FF6B0, rot_mat);
    }

    neg_pos[0] = -SW(cam, 0x00);
    neg_pos[1] = -SW(cam, 0x04);
    neg_pos[2] = -SW(cam, 0x08);
    func_800B4E48(D_800FF6B0, (unsigned char *)neg_pos, D_800FF6B0 + 0x14);

    if (SW(cam, 0x1C) != 0) {
        /* UNCLEAR: exact semantics of cam+0x1C and rot_mat+0x14 (sp+0x44) passed to func_800B4E48 */
        func_800B67B8(SW(cam, 0x1C), rot_mat);
        func_800B6548(rot_mat, tmp_mat);
        func_800B4E48(tmp_mat, rot_mat + 0x14, (unsigned char *)neg_pos);
        SW(tmp_mat, 0x14) = -neg_pos[0];
        SW(tmp_mat, 0x18) = -neg_pos[1];
        SW(tmp_mat, 0x1C) = -neg_pos[2];
        func_800B4B94(D_800FF6B0, tmp_mat);
        copy32(D_800FF6B0, tmp_mat);
    }

    copy32(D_800EF8F8, D_800FF6B0);
    return 0;
}
