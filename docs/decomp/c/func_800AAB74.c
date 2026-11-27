/*
 * func_800AAB74
 *
 * Purpose:
 *   Queues or stores sound/event IDs after subtracting base offset 0xBE8 (3048)
 *   according to ID range:
 *   - 0x0BE9..0x13B7 (3049..5047): appends `(id - 0xBE8)` into 32-entry ring
 *     buffer `D_800CE7B8`, advancing and wrapping index `D_800E6A9A` mod 32.
 *   - 0x13B9..0x151A (5049..5402): stores `(u16)(id - 0xBE8)` into `D_800E6A90`.
 *   - 0x17A1..0x17A7 (6049..6055): stores `(u32)(id - 0xBE8)` into `D_800E6A94`.
 *
 * Arguments:
 *   eventId (unsigned int) - raw event/sound ID (low 16 bits checked)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800E6A9A (s16)     ring buffer write index (0..31)
 *   D_800CE7B8 (u16[32]) queued event ID ring buffer
 *   D_800E6A90 (u16)     range 2 slot
 *   D_800E6A94 (u32)     range 3 slot
 */

extern signed short D_800E6A9A;
extern unsigned short D_800CE7B8[];
extern unsigned short D_800E6A90;
extern unsigned int D_800E6A94;

void func_800AAB74(unsigned int eventId)
{
    unsigned short id = (unsigned short)eventId;

    if ((unsigned short)(eventId - 0xBE9U) < 0x7CFU) {
        signed short idx = D_800E6A9A;
        signed short nextIdx = (signed short)(idx + 1);

        D_800E6A9A = nextIdx;
        D_800CE7B8[idx] = (unsigned short)(id - 0xBE8U);
        if (nextIdx >= 0x20) {
            D_800E6A9A = 0;
        }
    } else if ((unsigned short)(eventId - 0x13B9U) < 0x162U) {
        D_800E6A90 = (unsigned short)(id - 0xBE8U);
    } else if ((unsigned short)(eventId - 0x17A1U) < 0x7U) {
        D_800E6A94 = (unsigned int)(unsigned short)(id - 0xBE8U);
    }
}
