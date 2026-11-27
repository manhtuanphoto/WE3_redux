/*
 * func_8001BC30
 *
 * Purpose:
 *   Initializes/binds skeletal limb or render part entries for an actor.
 *   Calls func_800B4978(0, actor + 0x3C), clears the entry counter at
 *   actor + 0x38C, and iterates over 8-byte descriptor records at *(actor + 0x20)
 *   until reaching a sentinel byte of 0xFF, calling func_8001BCF4 for each entry.
 *
 * Arguments:
 *   actor (u8*) - pointer to actor struct:
 *     +0x020 (u32, R)   pointer to 0xFF-terminated array of 8-byte descriptor entries
 *     +0x03C (u8[], R)  actor local transform/matrix buffer
 *     +0x38C (u8, W/R)  count of processed entries
 *
 * Return value:
 *   None (void)
 *
 * Globals read:
 *   D_80139E00 (u32[]) - lookup table used when entry byte 0 >= 0x80
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned int D_80139E00[];

extern void func_800B4978(unsigned int arg0, unsigned char *buf);
extern void func_8001BCF4(unsigned char *actor, unsigned int idx, unsigned int data);

void func_8001BC30(unsigned char *actor)
{
    unsigned char *entry;
    unsigned int idx;

    func_800B4978(0, actor + 0x3C);

    entry = (unsigned char *)W(actor, 0x20);
    B(actor, 0x38C) = 0;
    idx = 0;

    while (entry[0] != 0xFF) {
        unsigned int tag = entry[0];
        unsigned int data;

        if (tag < 0x80U) {
            data = W(entry, 4);
        } else {
            data = D_80139E00[tag];
        }

        func_8001BCF4(actor, idx & 0xFFU, data);

        entry += 8;
        B(actor, 0x38C) = (unsigned char)(B(actor, 0x38C) + 1);
        idx++;
    }
}
