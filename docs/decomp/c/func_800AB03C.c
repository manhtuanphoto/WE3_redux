/*
 * func_800AB03C
 *
 * Purpose:
 *   Snapshots the 39 (0x27) per-channel base volume bytes from the 8-byte-stride
 *   channel parameter table `D_800CE86C[i * 8 + 0]` into `D_800F1040[i]`.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800CE86C (unsigned char[39][8]): channel parameter table (+0x00 base volume)
 *   D_800F1040 (unsigned char[39]):    saved per-channel base volume snapshot
 */

extern unsigned char D_800CE86C[];
extern unsigned char D_800F1040[];

void func_800AB03C(void)
{
    int i;

    for (i = 0; i < 0x27; i++) {
        D_800F1040[i] = D_800CE86C[i << 3];
    }
}
