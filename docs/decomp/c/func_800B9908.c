/*
 * func_800B9908 (and trailing unlabelled function sub_800B993C)
 *
 * Purpose:
 *   func_800B9908: Clears the first 32-bit word of `count` consecutive 32-byte entries
 *   in the table pointed to by D_8010CD8C starting at `start_idx`.
 *
 *   sub_800B993C (unlabelled function immediately following jr $ra in func_800B9908.s):
 *   Advances/consumes a slot at D_8010C1CC in D_8010CD8C (32-byte stride). If slot status
 *   (+0x00 u16) == 1 and D_8010CD58 != 0, clears status and resets D_8010C1CC to 0.
 *   If the resulting slot status == 2, sets status to 4, writes pointers to *out_a and
 *   *out_b (using D_8010CE68 * 32 + D_8010C1CC * 2016), and returns 0; otherwise returns 1.
 *
 * Arguments:
 *   func_800B9908:
 *     start_idx (unsigned int): starting entry index in D_8010CD8C
 *     count (unsigned int): number of entries to clear
 *   sub_800B993C:
 *     out_a (unsigned int *): receives pointer into D_8010CD8C buffer
 *     out_b (unsigned int *): receives pointer to active 32-byte slot
 *
 * Return value:
 *   func_800B9908: void
 *   sub_800B993C: 0 if status == 2 was transitioned to 4, else 1
 *
 * Global fields read/written:
 *   D_8010CD8C (unsigned char *): base of 32-byte entry table
 *   D_8010C1CC (unsigned int): current slot index
 *   D_8010CD58 (unsigned int): reset-enable flag
 *   D_8010CE68 (unsigned int): sub-slot index
 */

#define H(p, o) (*(unsigned short *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned char *D_8010CD8C;
extern unsigned int   D_8010C1CC;
extern unsigned int   D_8010CD58;
extern unsigned int   D_8010CE68;

void func_800B9908(unsigned int start_idx, unsigned int count)
{
    unsigned int i = 0;

    if (count == 0) {
        return;
    }

    do {
        W(D_8010CD8C, (i + start_idx) << 5) = 0;
        i++;
    } while (i < count);
}

/* UNCLEAR: unlabelled second function following jr $ra at 0x800B993C in func_800B9908.s */
unsigned int sub_800B993C(unsigned int *out_a, unsigned int *out_b)
{
    unsigned char *slot = D_8010CD8C + (D_8010C1CC << 5);

    if (H(slot, 0) == 1) {
        unsigned int flag = D_8010CD58;
        D_8010C1CC = 0;
        if (flag != 0) {
            H(slot, 0) = 0;
        }
        slot = D_8010CD8C + (D_8010C1CC << 5);
    }

    if (H(slot, 0) == 2) {
        H(slot, 0) = 4;
        *out_a = (unsigned int)(D_8010CD8C + (D_8010CE68 << 5) + (D_8010C1CC * 2016U));
        *out_b = (unsigned int)slot;
        return 0;
    }

    return 1;
}
