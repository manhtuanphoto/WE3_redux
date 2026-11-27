/*
 * func_800B58C0
 *
 * Purpose:
 *   Copies a 32-byte (8-word) structure from src into the global buffer
 *   D_800F1078 and then invokes func_800B5978().
 *
 * Arguments:
 *   src (const unsigned int *): pointer to 32-byte source structure
 *
 * Return value:
 *   int: return value forwarded from func_800B5978() (if any; void if unused)
 *
 * Global fields written:
 *   D_800F1078 (u32[8]): 32-byte staging buffer
 */

extern unsigned int D_800F1078[8];

extern int func_800B5978(void);

int func_800B58C0(const unsigned int *src)
{
    D_800F1078[0] = src[0];
    D_800F1078[1] = src[1];
    D_800F1078[2] = src[2];
    D_800F1078[3] = src[3];
    D_800F1078[4] = src[4];
    D_800F1078[5] = src[5];
    D_800F1078[6] = src[6];
    D_800F1078[7] = src[7];

    return func_800B5978();
}
