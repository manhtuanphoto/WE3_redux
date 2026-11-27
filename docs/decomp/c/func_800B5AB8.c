/*
 * func_800B5AB8
 *
 * Purpose:
 *   Splices an Ordering Table (OT) / GPU primitive list (`srcOt`) into a
 *   destination OT (`dstOt`) at index `(srcOt->+0x0C - dstOt->+0x08)` (PsyQ
 *   GsSortOt-style OT merge). Walks `srcOt->+0x04` while 24-bit next links !=
 *   0x00FFFFFF, keeping a trailing pointer (`spliceNode`) and links it to the
 *   destination OT entry while preserving each tag's upper 8-bit field
 *   (0xFF000000).
 *
 * Arguments:
 *   srcOt (unsigned char *) - source OT struct (+0x04 head tag ptr, +0x0C point/index, +0x10 tail/link ptr)
 *   dstOt (unsigned char *) - destination OT struct (+0x04 org table ptr, +0x08 offset)
 *
 * Return value:
 *   (unsigned char *) dstOt
 *
 * Struct fields read:
 *   srcOt+0x04 (u32 *)  starting tag pointer
 *   srcOt+0x0C (u32)    slot/depth index
 *   srcOt+0x10 (u32)    24-bit link address spliced into dstOt entry
 *   dstOt+0x04 (u32 *)  OT tag array base
 *   dstOt+0x08 (u32)    base offset subtracted from srcOt+0x0C
 *
 * Notes:
 *   Due to the trailing pointer update in the traversal loop (`spliceNode = prev;
 *   prev = cur; cur = next;`), when the list has more than one link before the
 *   0x00FFFFFF terminator, `spliceNode` lags two steps behind `cur`.
 */

#define W(p, o) (*(unsigned int *)((p) + (o)))

unsigned char *func_800B5AB8(unsigned char *srcOt, unsigned char *dstOt)
{
    unsigned int *cur = (unsigned int *)W(srcOt, 0x04);
    unsigned int *prev = cur;
    unsigned int *spliceNode = cur;
    unsigned int slotIndex = W(srcOt, 0x0C) - W(dstOt, 0x08);
    unsigned int *dstBase = (unsigned int *)W(dstOt, 0x04);
    unsigned int *dstEntry;

    if ((*spliceNode & 0x00FFFFFFU) != 0x00FFFFFFU) {
        do {
            spliceNode = prev;
            prev = cur;
            cur = (unsigned int *)(*cur & 0x00FFFFFFU);
        } while ((*cur & 0x00FFFFFFU) != 0x00FFFFFFU);
    }

    dstEntry = dstBase + slotIndex;
    *spliceNode = (*spliceNode & 0xFF000000U) | (*dstEntry & 0x00FFFFFFU);
    *dstEntry = (*dstEntry & 0xFF000000U) | (W(srcOt, 0x10) & 0x00FFFFFFU);

    return dstOt;
}
