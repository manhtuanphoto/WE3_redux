/*
 * func_800477A0
 *
 * Purpose:
 *   Finds up to `count` closest eligible player actors (out of the 22 players
 *   in D_800FF748[1..22], 1000-byte stride) to the 2D (x, z) position of `ref`.
 *   Then updates player selection/marking fields (+0x39C, +0x3A5, +0x3A6, +0x3D3)
 *   for all active players depending on whether they are among the `count` closest.
 *
 * Arguments:
 *   ref   - pointer to reference entity with s16 x at +0x2 and s16 z at +0x6
 *   count - maximum number of closest players to select (masked to u8; max 16
 *           due to stack buffer size)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read / written:
 *   D_800FF748 (actor table base, 1000-byte stride; indices 1..22 are players)
 *     +0x000 (u8) active flag
 *     +0x002 (s16) x position
 *     +0x006 (s16) z position
 *     +0x38D (u8) player ID
 *     +0x39C (u8) selected/closest flag (1 if in top `count`, 0 otherwise)
 *     +0x3A5 (u8) = 12 if count == 0, else 0
 *     +0x3A6 (u8) = 12 if in top `count`, 24 otherwise
 *     +0x3D2 (u8) exclusion flag (skipped from distance search if != 0)
 *     +0x3D3 (u8) = 1 if in top `count`, 0 otherwise
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define SH(p, o) (*(short *)((p) + (o)))

extern unsigned char D_800FF748[];

void func_800477A0(const unsigned char *ref, unsigned int count)
{
    unsigned char best_ids[16];
    unsigned int best_dist[16];
    unsigned int n = count & 0xFF;
    unsigned int i, j, k;
    unsigned char *actor;

    for (i = 0; i < n; i++) {
        best_dist[i] = 0xFFFFFFFFU;
    }

    actor = D_800FF748 + 0x3E8; /* player 1 (stride 1000 = 0x3E8) */
    for (i = 0; i < 22; i++, actor += 0x3E8) {
        int dx, dz;
        unsigned int dist_sq;

        if (B(actor, 0x000) == 0) {
            continue;
        }
        if (B(actor, 0x3D2) != 0) {
            continue;
        }

        dx = (int)SH(ref, 0x02) - (int)SH(actor, 0x02);
        dz = (int)SH(ref, 0x06) - (int)SH(actor, 0x06);
        dist_sq = (unsigned int)(dx * dx + dz * dz);

        if (n == 0) {
            continue;
        }

        for (j = 0; j < n; j++) {
            if (dist_sq < best_dist[j]) {
                for (k = n - 1; (int)j < (int)k; k--) {
                    best_ids[k] = best_ids[k - 1];
                    best_dist[k] = best_dist[k - 1];
                }
                best_ids[j] = B(actor, 0x38D);
                best_dist[j] = dist_sq;
                break;
            }
        }
    }

    actor = D_800FF748 + 0x3E8;
    for (i = 0; i < 22; i++, actor += 0x3E8) {
        unsigned char id;

        if (B(actor, 0x000) == 0) {
            continue;
        }

        B(actor, 0x3A5) = 12;
        B(actor, 0x3A6) = 24;
        B(actor, 0x3D3) = 0;
        B(actor, 0x39C) = 0;

        if (n == 0) {
            continue;
        }

        id = B(actor, 0x38D);
        for (j = 0; j < n; j++) {
            /* Delay slot of bne unconditionally clears +0x3A5 when n > 0 */
            B(actor, 0x3A5) = 0;
            if (id == best_ids[j]) {
                B(actor, 0x3A6) = 12;
                B(actor, 0x3D3) = 1;
                B(actor, 0x39C) = 1;
                break;
            }
        }
    }
}
