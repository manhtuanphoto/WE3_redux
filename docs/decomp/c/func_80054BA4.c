/*
 * func_80054BA4
 *
 * Purpose:
 *   Validates a player/actor ID against the team of `actor` (`actor+0x399`):
 *   - Returns 0 if `(playerId & 0xFF) == 0`.
 *   - If `actor+0x399 == 0`, returns `playerId & 0xFF` when it is in 1..11 (< 0xC).
 *   - If `actor+0x399 != 0`, returns `playerId & 0xFF` when `(u8)(playerId - 12) < 11` (12..22).
 *   - Otherwise calls `func_80049D58()` (RNG), reduces `(rng & 0xFF) % 24` via
 *     reciprocal multiply (`0xAAAAAAAB`), and returns `D_800FF748[idx * 1000 + 0x38D]`.
 *
 * Arguments:
 *   actor    (unsigned char *) - reference actor struct (+0x399 u8 team side)
 *   playerId (unsigned int)    - candidate player/actor ID (low 8 bits)
 *
 * Return value:
 *   (unsigned int) 0, validated playerId (1..22), or byte +0x38D of a random actor in D_800FF748.
 *
 * Global / actor fields read:
 *   actor+0x399 (u8) team side (0 vs non-zero)
 *   D_800FF748  (u8[24][1000]) actor table, reads +0x38D (u8)
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))

extern unsigned char D_800FF748[];

extern unsigned int func_80049D58(void);

unsigned int func_80054BA4(unsigned char *actor, unsigned int playerId)
{
    unsigned int id = playerId & 0xFFU;
    unsigned int inTeam;
    unsigned int rng;
    unsigned int idx;

    if (id == 0U) {
        return 0U;
    }

    if (B(actor, 0x399) == 0) {
        inTeam = (id < 0xCU);
    } else {
        inTeam = (((playerId - 0xCU) & 0xFFU) < 0xBU);
    }

    if (inTeam) {
        return id;
    }

    rng = func_80049D58() & 0xFFU;
    idx = (rng % 24U) & 0xFFU;
    return B(D_800FF748 + idx * 1000U, 0x38D);
}
