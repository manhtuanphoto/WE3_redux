/*
 * func_800AEACC
 *
 * Purpose:
 *   Synchronizes the GPU driver via callback D_800CEABC->+0x3C(0) and then
 *   submits the command payload of a GPU primitive packet (starting at prim+0x04,
 *   with word count prim[0x03]) through D_800CEABC->+0x14.
 *
 * Arguments:
 *   prim (unsigned char *): pointer to GPU primitive packet
 *     +0x03 (u8)  packet word count
 *     +0x04 (...) command words
 *
 * Return value:
 *   int: return value from D_800CEABC->+0x14 (UNCLEAR: may be void if unused by callers)
 *
 * Global fields read:
 *   D_800CEABC (unsigned char *): pointer to GPU driver dispatch table
 *     +0x14 (fn ptr): submit primitive command words(words, count)
 *     +0x3C (fn ptr): GPU sync / wait callback(mode)
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned char *D_800CEABC;

int func_800AEACC(unsigned char *prim)
{
    unsigned int count = B(prim, 0x03);
    void (*syncFn)(int) = (void (*)(int))W(D_800CEABC, 0x3C);
    int (*submitFn)(unsigned char *, unsigned int);

    syncFn(0);

    submitFn = (int (*)(unsigned char *, unsigned int))W(D_800CEABC, 0x14);
    return submitFn(prim + 0x04, count);
}
