/*
 * func_800BAE58 - Initialize VSync / interrupt callback table and return setter
 *
 * Purpose:
 *   Initializes the 8-slot callback array D_800D4E94, writes 0x107 to *D_800D4EB8,
 *   clears the tick counter D_800D4EB4, registers the internal dispatcher
 *   D_800BAEB0 via func_800BA768(0, D_800BAEB0), and returns a pointer to the
 *   internal slot setter D_800BAF1C.
 *
 *   Note: The assembly file contains two additional internal labels (`alabel`):
 *     - D_800BAEB0: callback dispatcher that increments D_800D4EB4 and invokes
 *       each non-NULL function pointer in D_800D4E94[0..7].
 *     - D_800BAF1C(idx, cb): writes `cb` into D_800D4E94[idx].
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   void * : pointer to internal callback setter D_800BAF1C
 *
 * Globals read & written:
 *   D_800D4E94 (void (*[8])(void), R/W): 8-slot callback table
 *   D_800D4EB4 (u32, R/W): callback invocation / frame counter
 *   D_800D4EB8 (u32 *, R): pointer set to 0x107 on init
 */

typedef void (*vsync_cb_t)(void);

extern vsync_cb_t D_800D4E94[8];
extern unsigned int D_800D4EB4;
extern unsigned int *D_800D4EB8;

extern void func_800BAF48(vsync_cb_t *table, int count);
extern void func_800BA768(int spec, vsync_cb_t cb);

static void D_800BAEB0(void)
{
    int i;

    D_800D4EB4 += 1u;
    for (i = 0; i < 8; i++) {
        if (D_800D4E94[i] != 0) {
            D_800D4E94[i]();
        }
    }
}

static vsync_cb_t D_800BAF1C(int idx, vsync_cb_t cb)
{
    vsync_cb_t prev = D_800D4E94[idx];

    /* Assembly executes `sw $a1, 0($a0)` in the beq delay slot unconditionally */
    D_800D4E94[idx] = cb;
    return prev;
}

void *func_800BAE58(void)
{
    *D_800D4EB8 = 0x107u;
    D_800D4EB4 = 0u;
    func_800BAF48(D_800D4E94, 8);
    func_800BA768(0, D_800BAEB0);
    return (void *)D_800BAF1C;
}
