/*
 * func_8006EF90
 *
 * Purpose:
 *   Scans controller input ports against an 18-entry table of 16-bit button masks
 *   (D_800C8544[0..17]) using func_800174E0(port, 0, mask), returning the first
 *   matching mask or 0 if none match. Port range is controlled by Scratchpad+0x2E2:
 *   0 scans port 0 only; 1..4 scans ports 0..1.
 *
 * Arguments:
 *   start_port (unsigned char): starting port index (used only if Scratchpad+0x2E2 >= 5)
 *
 * Return value:
 *   First matching 16-bit button mask from D_800C8544, or 0 if none matched.
 *
 * Global fields read:
 *   D_800C8544 (const unsigned short[18]): button mask table
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x2E2 (u8): controller mode/port count selector
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern const unsigned short D_800C8544[18];

extern unsigned int func_800174E0(unsigned int port, unsigned int pad, unsigned short mask);

unsigned short func_8006EF90(unsigned char start_port)
{
    unsigned char mode = B(Scratchpad, 0x2E2);
    unsigned char port;
    unsigned char max_port;

    if (mode == 0) {
        port = 0;
        max_port = 1;
    } else if (mode < 5) {
        port = 0;
        max_port = 2;
    } else {
        /* UNCLEAR: in asm, $s3 (max_port) is left uninitialized when mode >= 5 */
        port = start_port;
        max_port = 0;
    }

    while ((port & 0xFF) < (max_port & 0xFF)) {
        unsigned int i = 0;
        do {
            unsigned short mask = D_800C8544[i & 0xFF];
            if (func_800174E0(port & 0xFF, 0, mask) != 0) {
                return mask;
            }
            i++;
        } while ((i & 0xFF) < 0x12U);
        port++;
    }

    return 0;
}
