/*
 * func_800B80BC
 *
 * Purpose:
 *   PsyQ low-level SIO / memory-card command transfer helper. Validates the
 *   channel and command buffer, writes the command bytes to the hardware FIFO
 *   pointer, starts the transfer, and (if synchronous) polls for completion
 *   with a timeout while dispatching registered callbacks.
 *
 * Arguments:
 *   chan      - channel/command ID (low byte used)
 *   cmdBuf    - pointer to command payload bytes
 *   outBuf    - optional 8-byte output buffer (copied from D_800E71C8 if non-NULL)
 *   asyncFlag - if non-zero, returns 0 immediately without polling
 *
 * Return value:
 *   0 on success (or async start), -2 if cmdBuf is NULL when payload length
 *   is non-zero, -1 on timeout or status error (5).
 *
 * Reads / Writes:
 *   Globals:
 *     D_800D3958 (s32) debug log level (R)
 *     D_800D3974 (u32[]) channel name string table (R)
 *     D_800D39F4 (u32[]) status name string table (R)
 *     D_800D3A94 (u32[]) secondary status enable flags per channel (R)
 *     D_800D3B94 (s32[]) command byte length per channel (R)
 *     D_800D3968 (u8[4]) cached 4-byte command header for chan == 2 (W)
 *     D_800D396C (u8) cached byte for chan == 0xE (W)
 *     D_800D396D (u8) active channel byte (R/W)
 *     D_800D3C14 (u8**) pointer to hardware status register byte (R/W target)
 *     D_800D3C18 (u8**) pointer to hardware command/port select register byte (W target)
 *     D_800D3C1C (u8**) pointer to hardware TX FIFO byte register (W target)
 *     D_800D3C2C (u8[2]) transfer completion status bytes (R/W)
 *     D_800D394C / D_800D3950 (void(*)(u32, u8*)) completion callbacks (R)
 *     D_800E71C8 (u8[8]) primary response buffer (R)
 *     D_800E71D0 (u8[]) secondary response buffer (R)
 *     D_800E71E0 (s32) timeout deadline tick (R/W)
 *     D_800E71E4 (s32) poll iteration counter (R/W)
 *     D_800E71E8 (u32) timeout label string pointer (R/W)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x3C0 = 960 ticks timeout window from func_800BA518(-1)
 *   0x3C0000 = 3,932,160 max poll loop iterations
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800152C8[];
extern unsigned char D_800152D8[];
extern unsigned char D_80015364[];
extern unsigned char D_8001536C[];
extern unsigned char D_8001537C[];
extern unsigned char D_800D394C[];
extern unsigned char D_800D3950[];
extern unsigned char D_800D3958[];
extern unsigned char D_800D3968[];
extern unsigned char D_800D396C[];
extern unsigned char D_800D396D[];
extern unsigned char D_800D3974[];
extern unsigned char D_800D39F4[];
extern unsigned char D_800D3A94[];
extern unsigned char D_800D3B94[];
extern unsigned char D_800D3C14[];
extern unsigned char D_800D3C18[];
extern unsigned char D_800D3C1C[];
extern unsigned char D_800D3C2C[];
extern unsigned char D_800E71C8[];
extern unsigned char D_800E71D0[];
extern unsigned char D_800E71E0[];
extern unsigned char D_800E71E4[];
extern unsigned char D_800E71E8[];

extern void func_800AD698(unsigned char*, ...);
extern void func_800B7B74(int, int);
extern int func_800BA518(int);
extern void func_800B8D80(unsigned char*);
extern void func_800B8550(void);
extern int func_800BA88C(void);
extern int func_800B7618(void);

int func_800B80BC(unsigned int chan, unsigned char* cmdBuf, unsigned char* outBuf, int asyncFlag) {
    unsigned int chanByte;
    int txLen;
    int i;

    if (SW(D_800D3958, 0) >= 2) {
        func_800AD698(D_80015364, W(D_800D3974, (chan & 0xFFu) * 4u));
    }

    chanByte = chan & 0xFFu;
    if (W(D_800D3B94, chanByte * 4u) != 0 && cmdBuf == 0) {
        if (SW(D_800D3958, 0) > 0) {
            func_800AD698(D_8001536C, W(D_800D3974, chanByte * 4u));
        }
        return -2;
    }

    func_800B7B74(0, 0);

    chanByte = chan & 0xFFu;
    if (chanByte == 2u) {
        for (i = 0; i < 4; i++) {
            B(D_800D3968, i) = B(cmdBuf, i);
        }
        chanByte = chan & 0xFFu;
    }
    if (chanByte == 0xEu) {
        B(D_800D396C, 0) = B(cmdBuf, 0);
    }

    B(D_800D3C2C, 0) = 0;
    if (W(D_800D3A94, chanByte * 4u) != 0) {
        B(D_800D3C2C, 1) = 0;
    }

    B((unsigned char*)W(D_800D3C14, 0), 0) = 0;

    txLen = SW(D_800D3B94, chanByte * 4u);
    if (txLen > 0) {
        for (i = 0; i < txLen; i++) {
            B((unsigned char*)W(D_800D3C1C, 0), 0) = B(cmdBuf, i);
        }
    }

    B(D_800D396D, 0) = (unsigned char)chan;
    B((unsigned char*)W(D_800D3C18, 0), 0) = (unsigned char)chan;

    if (asyncFlag != 0) {
        return 0;
    }

    SW(D_800E71E0, 0) = func_800BA518(-1) + 0x3C0;
    SW(D_800E71E4, 0) = 0;
    W(D_800E71E8, 0) = (unsigned int)D_8001537C;

    while (B(D_800D3C2C, 0) == 0) {
        int timedOut = 0;
        if (SW(D_800E71E0, 0) < func_800BA518(-1)) {
            timedOut = 1;
        } else {
            int count = SW(D_800E71E4, 0);
            SW(D_800E71E4, 0) = count + 1;
            if (count > 0x3C0000) {
                timedOut = 1;
            }
        }

        if (timedOut) {
            func_800B8D80(D_800152C8);
            func_800AD698(
                D_800152D8,
                W(D_800E71E8, 0),
                W(D_800D3974, (unsigned int)B(D_800D396D, 0) * 4u),
                W(D_800D39F4, (unsigned int)B(D_800D3C2C, 0) * 4u),
                W(D_800D39F4, (unsigned int)B(D_800D3C2C, 1) * 4u)
            );
            func_800B8550();
            return -1;
        }

        if (func_800BA88C() != 0) {
            unsigned int savedLow = B((unsigned char*)W(D_800D3C14, 0), 0) & 3u;
            for (;;) {
                int ev = func_800B7618();
                if (ev == 0) {
                    break;
                }
                if ((ev & 4) != 0) {
                    unsigned int cb = W(D_800D3950, 0);
                    if (cb != 0) {
                        ((void (*)(unsigned int, unsigned char*))cb)(B(D_800D3C2C, 1), D_800E71D0);
                    }
                }
                if ((ev & 2) != 0) {
                    unsigned int cb = W(D_800D394C, 0);
                    if (cb != 0) {
                        ((void (*)(unsigned int, unsigned char*))cb)(B(D_800D3C2C, 0), D_800E71C8);
                    }
                }
            }
            B((unsigned char*)W(D_800D3C14, 0), 0) = (unsigned char)savedLow;
        }
    }

    if (outBuf != 0) {
        for (i = 0; i < 8; i++) {
            B(outBuf, i) = B(D_800E71C8, i);
        }
    }

    if (B(D_800D3C2C, 0) == 5) {
        return -1;
    }
    return 0;
}
