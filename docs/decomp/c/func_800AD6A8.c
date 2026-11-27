/*
 * func_800AD6A8
 *
 * Purpose:
 *   Recursive Quicksort implementation (analogous to standard C qsort) using
 *   func_800AD7F8(a, b, elemSize) to swap elements.
 *   Note: For count == 2, compares base and base + elemSize directly and swaps
 *   if cmp(base, base + elemSize) > 0. For count > 2, swaps the middle element
 *   (count >> 1) into base as the pivot, partitions elements < pivot, restores
 *   the pivot, and recurses on left and right subarrays.
 *
 * Arguments:
 *   base     (unsigned char *): array base pointer
 *   count    (unsigned int): number of elements
 *   elemSize (unsigned int): size of each element in bytes
 *   cmp      (signed int (*)(const void *, const void *)): comparator callback
 *
 * Return value:
 *   void
 */

extern void func_800AD7F8(void *a, void *b, unsigned int size);

void func_800AD6A8(
    unsigned char *base,
    unsigned int count,
    unsigned int elemSize,
    signed int (*cmp)(const void *, const void *))
{
    unsigned char *cur = base + elemSize;
    unsigned char *last;
    unsigned int leftCount = 0;
    unsigned int i;

    if (count < 2U) {
        return;
    }

    if (count == 2U) {
        if (cmp(base, cur) > 0) {
            func_800AD7F8(base, cur, elemSize);
        }
        return;
    }

    last = base;
    func_800AD7F8(base, base + (count >> 1) * elemSize, elemSize);

    for (i = 1U; i < count; i++, cur += elemSize) {
        if (cmp(cur, base) < 0) {
            last += elemSize;
            leftCount++;
            if (cur != last) {
                func_800AD7F8(cur, last, elemSize);
            }
        }
    }

    if (last != base) {
        func_800AD7F8(base, last, elemSize);
    }

    func_800AD6A8(base, leftCount, elemSize, cmp);
    func_800AD6A8(last + elemSize, count - leftCount - 1U, elemSize, cmp);
}
