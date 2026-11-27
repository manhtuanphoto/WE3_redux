/*
 * func_800B02E4 - Enqueue or immediately execute a GPU/DMA callback command
 *
 * Purpose:
 *   Attempts to execute `callback(data, arg3)` directly or enqueue it into a
 *   64-entry ring buffer (stride 0x60 = 96 bytes per entry, starting at D_8010A9B8).
 *   If the ring buffer is full, polls func_800B0AB4() and drains via func_800B0594()
 *   until space frees or func_800B0AB4() returns nonzero (returns -1).
 *   Under interrupt lock (func_800BA8B4), if immediate execution is allowed
 *   (D_800CEAC4[+0x01] == 0, or queue empty with *D_800CEBA8 bit 24 clear and
 *   D_800CEAC4[+0x0C] == 0), waits for *D_800CEB9C bit 26, invokes `callback`,
 *   restores interrupts, and returns 0. Otherwise registers func_800B0594 via
 *   func_800BA798(2, ...), copies `size` bytes (rounded up to words) into the
 *   entry's inline buffer D_8010A9C4 if `size != 0`, advances write index
 *   D_800CEBBC mod 64, drains via func_800B0594(), and returns queued count.
 *
 * Arguments:
 *   callback (void (*)(void *, void *)) - command execution callback
 *   data     (void *)                  - data pointer (or source buffer if size != 0)
 *   size     (int)                     - byte size to copy inline into queue slot (0 = store pointer directly)
 *   arg3     (void *)                  - second argument passed to callback
 *
 * Return value:
 *   int - 0 if executed immediately, -1 if aborted by func_800B0AB4(), or
 *         (D_800CEBBC - D_800CEBC0) & 0x3F queued entry count.
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned int D_800CEBBC;
extern unsigned int D_800CEBC0;
extern unsigned int D_800CEBC4;
extern unsigned char D_800CEAC4[];
extern volatile unsigned int *D_800CEBA8;
extern volatile unsigned int *D_800CEB9C;
extern unsigned int D_8010A9B8[];
extern unsigned int D_8010A9BC[];
extern unsigned int D_8010A9C0[];
extern unsigned int D_8010A9C4[];

extern void func_800B0A80(void);
extern int func_800B0AB4(void);
extern void func_800B0594(void);
extern unsigned int func_800BA8B4(unsigned int mask);
extern void func_800BA798(int irq_id, void (*handler)(void));

int func_800B02E4(void (*callback)(void *, void *), void *data, int size, void *arg3)
{
    func_800B0A80();

    while (((D_800CEBBC + 1) & 0x3F) == D_800CEBC0) {
        if (func_800B0AB4() != 0) {
            return -1;
        }
        func_800B0594();
    }

    D_800CEBC4 = func_800BA8B4(0);
    W(D_800CEAC4, 0x08) = 1;

    if (B(D_800CEAC4, 0x01) == 0 ||
        (D_800CEBBC == D_800CEBC0 &&
         (*D_800CEBA8 & 0x01000000U) == 0 &&
         W(D_800CEAC4, 0x0C) == 0)) {
        volatile unsigned int *status = D_800CEB9C;
        while ((*status & 0x04000000U) == 0) {
        }
        callback(data, arg3);
        func_800BA8B4(D_800CEBC4);
        return 0;
    }

    func_800BA798(2, func_800B0594);

    if (size != 0) {
        int word_count = ((size >= 0) ? size : (size + 3)) >> 2;
        unsigned int *src = (unsigned int *)data;
        unsigned int idx = D_800CEBBC;
        int i;

        for (i = 0; i < word_count; i++) {
            D_8010A9C4[D_800CEBBC * 24 + i] = src[i];
        }
        idx = D_800CEBBC;
        D_8010A9BC[idx * 24] = (unsigned int)&D_8010A9C4[idx * 24];
    } else {
        D_8010A9BC[D_800CEBBC * 24] = (unsigned int)data;
    }

    D_8010A9C0[D_800CEBBC * 24] = (unsigned int)arg3;
    D_8010A9B8[D_800CEBBC * 24] = (unsigned int)callback;

    D_800CEBBC = (D_800CEBBC + 1) & 0x3F;
    func_800BA8B4(D_800CEBC4);
    func_800B0594();

    return (int)((D_800CEBBC - D_800CEBC0) & 0x3F);
}
