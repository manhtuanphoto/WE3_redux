/*
 * func_800C441C
 *
 * Purpose:
 *   Queries the status of all 24 PS1 SPU voices (equivalent to PsyQ
 *   SpuGetAllKeysStatus). For each voice 0..23, combines its active/key-on bit
 *   in D_800D5118 with whether its current envelope volume at
 *   (*D_800D558C)[voice * 16 + 0x0C] is non-zero:
 *     - key on  & env != 0 -> 1 (SPU_ON)
 *     - key on  & env == 0 -> 3 (SPU_ON_ENV_OFF)
 *     - key off & env != 0 -> 2 (SPU_OFF_ENV_ON)
 *     - key off & env == 0 -> 0 (SPU_OFF)
 *
 * Arguments:
 *   status (unsigned char *) - output array of 24 voice status bytes
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800D558C (unsigned char *) - pointer to 24x 16-byte SPU voice structs (+0x0C u16 envx)
 *   D_800D5118 (unsigned int)    - 24-bit voice active/key-on bitmask
 */

#define H(p, o) (*(unsigned short *)((p) + (o)))

extern unsigned char *D_800D558C;
extern unsigned int D_800D5118;

void func_800C441C(unsigned char *status)
{
    int i;
    unsigned char *voice_base = D_800D558C;
    unsigned int key_mask = D_800D5118;

    for (i = 0; i < 24; i++) {
        unsigned short envx = H(voice_base + (i * 16), 0x0C);
        unsigned int key_on = key_mask & (1u << i);

        if (key_on != 0) {
            status[i] = (envx != 0) ? 1 : 3;
        } else {
            status[i] = (envx != 0) ? 2 : 0;
        }
    }
}
