/*
 * func_8006B904 - Compute defensive AI target coordinates (scratch 0xF8/0xFC) for a player
 *
 * Purpose:
 *   Updates the offside/defensive-line timer (D_800E6CB9), computes the base
 *   target X coordinate via func_8006B7F4, adjusts target (X, Z) according to
 *   the defensive tactic mode in scratch 0x30B (cases 0..5), scales Z by 4/5 for
 *   formations 18..19, and clamps X based on formation role (+0x391 = 0x11..0x19).
 *
 * Arguments:
 *   actor - pointer to 1000-byte player actor struct
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Actor fields read & written:
 *   Scratch 0xF8/FC (s16) - predicted/AI target x, z
 *   Scratch 0x2F4   (ptr) - ball actor pointer
 *   Scratch 0x2FD[2] (u8) - formation id per side
 *   Scratch 0x30B   (u8)  - defensive AI phase/mode (0..5)
 *   D_800E6CB8      (u8)  - actor id crossing into opponent half
 *   D_800E6CB9      (u8)  - half-crossing cooldown/timer (set to 0x14 = 20 frames)
 *   Actor +0x02     (s16) - x position
 *   Actor +0x38D    (u8)  - actor id
 *   Actor +0x391    (u8)  - formation role id (0x11..0x19 checked)
 *   Actor +0x398    (u8)  - side (0/1)
 *   Actor +0x399    (u8)  - attack direction flag (0/1)
 *
 * Magic numbers:
 *   0x14   (20)    = 0.67 s at 30 Hz (D_800E6CB9 timer)
 *   0x12F5 (4853)  = 19.412 m (ball |z| threshold for extra z tracking in roles 17..18)
 *   0x23E2 (9186)  = 36.744 m (x clamp for tactic modes 2 & 3)
 *   0x2E08 (11784) = 47.136 m (x clamp for tactic mode 4)
 *   0x2C08 (11272) = 45.088 m (x clamp for tactic mode 5)
 *   0x300  (768)   = 3.072 m (x offset in mode 4 and clamp for role 0x19)
 *   0x500  (1280)  = 5.12 m (x offset behind ball in mode 5)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char D_800E6CB8;
extern unsigned char D_800E6CB9;

extern int func_8005A76C(int cur, int target, int step);
extern int func_8005A7E4(int cur, int target, int step);
extern int func_8006B7F4(unsigned char *actor);
extern void func_800A1484(unsigned char *actor, unsigned char *px, unsigned char *pz, int scale);

void func_8006B904(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    int dir_sign;
    unsigned char formation;
    unsigned int role_idx;

    if (D_800E6CB9 != 0) {
        D_800E6CB9--;
        H(scratch, 0xFC) = (unsigned short)func_8005A76C(S(scratch, 0xFC), 0, 6);
    }

    dir_sign = (B(actor, 0x399) != 0) ? -1 : 1;
    if (S(actor, 2) * dir_sign > 0) {
        D_800E6CB9 = 0x14;
        D_800E6CB8 = B(actor, 0x38D);
    }

    H(scratch, 0xF8) = (unsigned short)func_8006B7F4(actor);

    switch (B(scratch, 0x30B)) {
    case 0:
        break;

    case 1: {
        unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
        H(scratch, 0xFC) = (unsigned short)func_8005A76C(S(scratch, 0xFC), S(ball, 6), 1);
        if (B(actor, 0x391) != 0x17) {
            func_800A1484(actor, scratch + 0xF8, scratch + 0xFC, 0x400);
        }
        break;
    }

    case 2: {
        unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
        int z = func_8005A76C(S(scratch, 0xFC), S(ball, 6), 1);
        int x;

        H(scratch, 0xFC) = (unsigned short)z;
        if ((unsigned int)(B(actor, 0x391) - 0x11) < 2) {
            int bz = S(ball, 6);
            int abs_bz = (bz < 0) ? -bz : bz;
            if (abs_bz >= 0x12F5) {
                H(scratch, 0xFC) = (unsigned short)func_8005A76C(z, bz, 2);
            }
        }
        x = S(scratch, 0xF8);
        if (((x < 0) ? -x : x) >= 0x23E3) {
            H(scratch, 0xF8) = (unsigned short)((x >= 0) ? 0x23E2 : -0x23E2);
        }
        func_800A1484(actor, scratch + 0xF8, scratch + 0xFC, 0x600);
        break;
    }

    case 3: {
        unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
        int z = func_8005A7E4(S(scratch, 0xFC), S(ball, 6), 0xF);
        int x;

        H(scratch, 0xFC) = (unsigned short)z;
        if ((unsigned int)(B(actor, 0x391) - 0x11) < 2) {
            int bz = S(ball, 6);
            int abs_bz = (bz < 0) ? -bz : bz;
            if (abs_bz >= 0x12F5) {
                H(scratch, 0xFC) = (unsigned short)func_8005A76C(z, bz, 2);
            }
        }
        x = S(scratch, 0xF8);
        if (((x < 0) ? -x : x) >= 0x23E3) {
            H(scratch, 0xF8) = (unsigned short)((x >= 0) ? 0x23E2 : -0x23E2);
        }
        func_800A1484(actor, scratch + 0xF8, scratch + 0xFC, 0x900);
        break;
    }

    case 4: {
        unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
        int z = func_8005A76C(S(scratch, 0xFC), 0, 2);
        int x;

        H(scratch, 0xFC) = (unsigned short)z;
        H(scratch, 0xF8) = (unsigned short)((S(scratch, 0xF8) + (S(ball, 2) - dir_sign * 0x300)) / 2);
        func_800A1484(actor, scratch + 0xF8, scratch + 0xFC, 0xC00);
        x = S(scratch, 0xF8);
        if (((x < 0) ? -x : x) >= 0x2E09) {
            H(scratch, 0xF8) = (unsigned short)((x >= 0) ? 0x2E08 : -0x2E08);
        }
        break;
    }

    case 5: {
        unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
        int x;

        H(scratch, 0xF8) = (unsigned short)(S(ball, 2) - dir_sign * 0x500);
        H(scratch, 0xFC) = (unsigned short)func_8005A76C(S(scratch, 0xFC), 0, 2);
        func_800A1484(actor, scratch + 0xF8, scratch + 0xFC, 0x800);
        x = S(scratch, 0xF8);
        if (((x < 0) ? -x : x) >= 0x2C09) {
            H(scratch, 0xF8) = (unsigned short)((x >= 0) ? 0x2C08 : -0x2C08);
        }
        break;
    }
    }

    formation = B(scratch, 0x2FD + B(actor, 0x398));
    if (formation >= 0x12 && formation < 0x14) {
        H(scratch, 0xFC) = (unsigned short)((S(scratch, 0xFC) * 4) / 5);
    }

    role_idx = (unsigned int)(B(actor, 0x391) - 0x11);
    switch (role_idx) {
    case 0: /* 0x11 */
    case 1: /* 0x12 */
    case 4: /* 0x15 */
    case 5: /* 0x16 */
    case 6: /* 0x17 */
    case 7: /* 0x18 */
        if (S(scratch, 0xF8) * dir_sign > 0) {
            H(scratch, 0xF8) = 0;
        }
        break;

    case 2: /* 0x13 */
    case 3: /* 0x14 */
        break;

    case 8: /* 0x19 */
        if (S(scratch, 0xF8) * dir_sign >= 0x301) {
            H(scratch, 0xF8) = (unsigned short)(dir_sign * 0x300);
        }
        break;
    }
}
