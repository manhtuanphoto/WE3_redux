/*
 * func_800B7DF4
 *
 * Purpose:
 *   Polls asynchronous controller/card events via func_800BA88C and func_800B7618,
 *   dispatching registered callbacks D_800D3950 / D_800D394C and returning an 8-byte
 *   result packet into outBuf when ready, or -1 on timeout (0x3C0 ticks or 0x3C0000 polls).
 *
 * Arguments:
 *   nonblocking: (int) if 0, loops until an event arrives or timeout; if nonzero, polls once
 *   outBuf:      (unsigned char*) optional 8-byte output buffer (may be NULL)
 *
 * Return value:
 *   (int) nonzero status byte (from D_800D3C2C[2] or [1]) on success, 0 if nonblocking
 *         and nothing ready, or -1 on timeout
 *
 * Reads / Writes:
 *   D_800E71E0 (s32): deadline timestamp = start + 0x3C0 (W/R)
 *   D_800E71E4 (s32): poll iteration counter, limit 0x3C0000 (W/R)
 *   D_800E71E8 (void*): debug/status string pointer D_80015358 (W/R)
 *   D_800D3C2C[0..2] (u8): channel/pending flags; [2] or [1] cleared on consume (R/W)
 *   D_800D3C14 (u8**): pointer to control byte masked with & 3 during callback dispatch (R/W)
 *   D_800D3950 / D_800D394C: callback function pointers (R)
 *   D_800E71D8 / D_800E71D0 (u8[8]): 8-byte response buffers copied to outBuf (R)
 *   D_800E71C8: buffer passed to D_800D394C (R)
 *
 * Magic numbers:
 *   0x3C0 (960): timeout ticks against func_800BA518(-1).
 *   0x3C0000 (3932160): maximum poll loop iterations before timeout error.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern signed int func_800BA518(int);
extern void func_800B8D80(const char *);
extern void func_800AD698(const char *, void *, void *, void *, void *);
extern void func_800B8550(void);
extern int func_800BA88C(void);
extern int func_800B7618(void);

extern signed int D_800E71E0;
extern signed int D_800E71E4;
extern void *D_800E71E8;
extern char D_80015358[];
extern char D_800152C8[];
extern char D_800152D8[];
extern unsigned char D_800D396D;
extern void *D_800D3974[];
extern void *D_800D39F4[];
extern unsigned char D_800D3C2C[];
extern unsigned char *D_800D3C14;
extern void (*D_800D3950)(unsigned char, void *);
extern void (*D_800D394C)(unsigned char, void *);
extern unsigned char D_800E71C8[];
extern unsigned char D_800E71D0[];
extern unsigned char D_800E71D8[];

int func_800B7DF4(int nonblocking, unsigned char *outBuf) {
    D_800E71E0 = func_800BA518(-1) + 0x3C0;
    D_800E71E4 = 0;
    D_800E71E8 = D_80015358;

    while (1) {
        signed int iter;
        unsigned char val;

        iter = D_800E71E4;
        D_800E71E4 = iter + 1;
        if (D_800E71E0 < func_800BA518(-1) || iter > 0x3C0000) {
            func_800B8D80(D_800152C8);
            func_800AD698(
                D_800152D8,
                D_800E71E8,
                D_800D3974[D_800D396D],
                D_800D39F4[D_800D3C2C[0]],
                D_800D39F4[D_800D3C2C[1]]
            );
            func_800B8550();
            return -1;
        }

        if (func_800BA88C() != 0) {
            unsigned char saved = *D_800D3C14 & 3u;
            int ev;
            while ((ev = func_800B7618()) != 0) {
                if ((ev & 4) != 0 && D_800D3950 != 0) {
                    D_800D3950(D_800D3C2C[1], D_800E71D0);
                }
                if ((ev & 2) != 0 && D_800D394C != 0) {
                    D_800D394C(D_800D3C2C[0], D_800E71C8);
                }
            }
            *D_800D3C14 = saved;
        }

        val = D_800D3C2C[2];
        if (val != 0) {
            D_800D3C2C[2] = 0;
            if (outBuf != 0) {
                int i;
                for (i = 0; i < 8; i++) {
                    outBuf[i] = D_800E71D8[i];
                }
            }
            return val;
        }

        val = D_800D3C2C[1];
        if (val != 0) {
            D_800D3C2C[1] = 0;
            if (outBuf != 0) {
                int i;
                for (i = 0; i < 8; i++) {
                    outBuf[i] = D_800E71D0[i];
                }
            }
            return val;
        }

        if (nonblocking != 0) {
            return 0;
        }
    }
}
