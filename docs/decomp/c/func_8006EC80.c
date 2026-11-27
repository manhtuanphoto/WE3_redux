/*
 * func_8006EC80
 *
 * Purpose:
 *   Polls controller button inputs (from Scratchpad+0x230 held and +0x234 newly
 *   pressed, 8-byte stride per pad) against the 18-entry button mask table
 *   D_800C8544[4..17], with D-pad auto-repeat (indices 4..7) managed via
 *   D_8010CD44 (timer) and D_800F10A0 (repeat count).
 *
 * Arguments:
 *   mode - pad selection (masked to u8):
 *            0: poll pad 0
 *            1: poll pad 1
 *            2: poll pads 0 and 1
 *            (>2: UNCLEAR: loop bounds are uninitialized in assembly)
 *
 * Return value:
 *   Matched button mask from D_800C8544, or 0 if no button triggered.
 *
 * Global fields read / written:
 *   D_800C8544 (u16[18] button bitmask table)
 *   D_8010CD44 (u16[2] repeat frame counters)
 *   D_800F10A0 (u16[2] repeat stage counters)
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x230 + pad*8 (u16) held buttons
 *   +0x234 + pad*8 (u16) newly pressed buttons
 *   +0x298 (u8) game state (if == 4, repeat period is 16 frames; else 8 frames)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned short D_800C8544[];
extern unsigned short D_8010CD44[];
extern unsigned short D_800F10A0[];

unsigned short func_8006EC80(unsigned int mode)
{
    unsigned int m = mode & 0xFF;
    unsigned int start_pad = 0; /* UNCLEAR: uninitialized in asm if m > 2 */
    unsigned int end_pad = 0;   /* UNCLEAR: uninitialized in asm if m > 2 */
    int div;
    unsigned int pad;

    if (m == 0) {
        start_pad = 0;
        end_pad = 1;
    } else if (m == 1) {
        start_pad = 1;
        end_pad = 2;
    } else if (m == 2) {
        start_pad = 0;
        end_pad = 2;
    }

    div = (B(Scratchpad, 0x298) == 4) ? 1 : 2;

    for (pad = start_pad & 0xFF; pad < (end_pad & 0xFF); pad++) {
        const unsigned char *pad_ptr = Scratchpad + pad * 8;
        unsigned int btn;

        for (btn = 4; btn < 18; btn++) {
            unsigned short mask = D_800C8544[btn];

            if ((mask & H(pad_ptr, 0x234)) != 0) {
                return mask;
            }

            if ((mask & H(pad_ptr, 0x230)) != 0 && btn < 8) {
                int limit = 16 / div;
                unsigned short timer = D_8010CD44[pad];

                if ((int)timer < limit) {
                    D_8010CD44[pad] = (D_800F10A0[pad] < 2)
                        ? (unsigned short)(timer + 1)
                        : (unsigned short)(timer + 3);
                    return 0;
                } else {
                    D_8010CD44[pad] = 0;
                    D_800F10A0[pad] = (unsigned short)(D_800F10A0[pad] + 1);
                    return D_800C8544[btn];
                }
            }
        }

        D_8010CD44[pad] = 0;
        D_800F10A0[pad] = 0;
    }

    return 0;
}
