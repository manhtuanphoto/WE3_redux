/*
 * func_8006F87C
 *
 * Purpose:
 *   Increments two signed 16-bit fields (+0x1E and +0x20) in entry `slot` of
 *   the 40-byte stride table D_80105508 by `step` up toward `target1` and
 *   `target2`, clamping each to its target. Returns 1 only when both fields
 *   have reached or exceeded their targets, otherwise 0.
 *
 * Arguments:
 *   slot    (u8 in int)  - index into D_80105508 (0x28-byte stride)
 *   target1 (s16 in int) - upper target for field +0x1E
 *   target2 (s16 in int) - upper target for field +0x20
 *   step    (s32)        - increment amount added per call
 *
 * Return value:
 *   int - 1 if both fields reached their targets, 0 otherwise
 *
 * Globals read/written:
 *   D_80105508 (u8[], R/W) - table of 40-byte (0x28) structs (+0x1E s16, +0x20 s16)
 */

#define S(p, o) (*(signed short *)((p) + (o)))

extern unsigned char D_80105508[];

int func_8006F87C(int slot, int target1, int target2, int step)
{
    unsigned char *entry = D_80105508 + (slot & 0xFF) * 0x28;
    int done1 = 0;
    int done2 = 0;
    signed short t1 = (signed short)target1;
    signed short t2 = (signed short)target2;
    signed short cur1 = S(entry, 0x1E);
    signed short cur2;

    if (cur1 < t1) {
        signed short next1 = (signed short)(cur1 + step);
        S(entry, 0x1E) = next1;
        if (next1 >= t1) {
            S(entry, 0x1E) = t1;
            done1 = 1;
        }
    } else {
        S(entry, 0x1E) = t1;
        done1 = 1;
    }

    cur2 = S(entry, 0x20);
    if (cur2 < t2) {
        signed short next2 = (signed short)(cur2 + step);
        S(entry, 0x20) = next2;
        if (next2 >= t2) {
            S(entry, 0x20) = t2;
            done2 = 1;
        }
    } else {
        S(entry, 0x20) = t2;
        done2 = 1;
    }

    return done2 & done1;
}
