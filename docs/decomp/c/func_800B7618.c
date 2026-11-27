/*
 * func_800B7618
 *
 * Purpose:
 *   PS1 PsyQ CD-ROM interrupt handler (CD_intr). Selects register bank 1,
 *   reads and acknowledges the CD-ROM interrupt status (*D_800D3C20 & 7),
 *   drains up to 8 result bytes from the response FIFO (*D_800D3C18), updates
 *   saved status/error globals (D_800D395C, D_800D3960, D_800D3964), logs
 *   DiskError on interrupt 5 (when D_800D3958 > 0), and dispatches on the
 *   interrupt type (1..5) to update CD state bytes (D_800D3C2C) and copy the
 *   result buffer (D_800E71C8 / D_800E71D0 / D_800E71D8).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   0 on spurious/empty interrupt (0) or unknown interrupt (>5);
 *   1 on intr 3 (Acknowledge) with zero status error and non-zero D_800D3A14[cmd];
 *   2 on intr 2 (Complete) or intr 3 without D_800D3A14[cmd] or with status error;
 *   4 on intr 1 (DataReady) or intr 4 (DataEnd);
 *   6 on intr 5 (DiskError).
 *
 * Global fields read:
 *   D_800D3C14 (u8 * CDROM HSTS/bank reg), D_800D3C18 (u8 * CDROM RDDATA),
 *   D_800D3C1C (u8 * CDROM HINTMSK), D_800D3C20 (u8 * CDROM HINTSTS),
 *   D_800D3958 (s32 debug/print level), D_800D395C (u32 last stat byte),
 *   D_800D3960 (u32 last error/result1 byte), D_800D3964 (u32 shell-open count),
 *   D_800D396D (u8 current CD command index),
 *   D_800D3974 (char *[] command names), D_800D3A14 (u32[] command table),
 *   D_800D3B14 (u32[] command table)
 *
 * Global fields written:
 *   D_800D395C, D_800D3960, D_800D3964,
 *   D_800D3C2C[0..2] (u8 CD state/result codes),
 *   D_800E71C8[8], D_800E71D0[8], D_800E71D8[8] (u8 result buffers)
 *
 * Magic numbers:
 *   No gameplay physics constants. Hardware masks: 0x07 (interrupt code),
 *   0x20 (HSTS RSLTEMPTY), 0x10 (shell open), 0x1D (status error bits).
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern volatile unsigned char *D_800D3C14;
extern volatile unsigned char *D_800D3C18;
extern volatile unsigned char *D_800D3C1C;
extern volatile unsigned char *D_800D3C20;
extern signed int D_800D3958;
extern unsigned int D_800D395C;
extern unsigned int D_800D3960;
extern unsigned int D_800D3964;
extern unsigned char D_800D396D;
extern unsigned int D_800D3974[];
extern unsigned int D_800D3A14[];
extern unsigned int D_800D3B14[];
extern unsigned char D_800D3C2C[];
extern unsigned char D_800E71C8[];
extern unsigned char D_800E71D0[];
extern unsigned char D_800E71D8[];
extern unsigned char D_800152F4[];
extern unsigned char D_80015300[];
extern unsigned char D_8001531C[];
extern unsigned char D_80015330[];

extern void func_800AD698(unsigned char *fmt, ...);
extern void func_800B8D80(unsigned char *msg);

int func_800B7618(void)
{
    unsigned char result[8];
    unsigned char intr;
    unsigned int errBits;
    int count;
    int i;

    *D_800D3C14 = 1;
    intr = *D_800D3C20 & 7;
    if (intr == 0) {
        return 0;
    }

    errBits = 0;
    while (intr != (*D_800D3C20 & 7)) {
        intr = *D_800D3C20 & 7;
    }

    count = 0;
    while ((*D_800D3C14 & 0x20) != 0) {
        result[count] = *D_800D3C18;
        count++;
        if (count >= 8) {
            break;
        }
    }

    for (i = count; i < 8; i++) {
        result[i] = 0;
    }

    *D_800D3C14 = 1;
    *D_800D3C20 = 7;
    *D_800D3C1C = 7;

    if (intr != 3 || D_800D3B14[D_800D396D] == 0) {
        if ((D_800D395C & 0x10) == 0 && (result[0] & 0x10) != 0) {
            D_800D3964++;
        }
        errBits = result[0] & 0x1D;
        D_800D395C = result[0];
        D_800D3960 = result[1];
    }

    if (intr == 5 && D_800D3958 > 0) {
        func_800AD698(D_800152F4);
        if (D_800D3958 > 0) {
            func_800AD698(D_80015300, D_800D3974[D_800D396D], D_800D395C, D_800D3960);
        }
    }

    switch (intr) {
    case 1:
        if (errBits != 0 && count == 1) {
            errBits = 0;
        }
        D_800D3C2C[1] = (errBits == 0) ? 1 : 5;
        if (D_800E71D0 != 0) {
            for (i = 0; i < 8; i++) {
                D_800E71D0[i] = result[i];
            }
        }
        *D_800D3C14 = 0;
        *D_800D3C20 = 0;
        return 4;

    case 2:
        D_800D3C2C[0] = (errBits == 0) ? 2 : 5;
        if (D_800E71C8 != 0) {
            for (i = 0; i < 8; i++) {
                D_800E71C8[i] = result[i];
            }
        }
        return 2;

    case 3:
        if (errBits != 0) {
            D_800D3C2C[0] = 5;
            if (D_800E71C8 != 0) {
                for (i = 0; i < 8; i++) {
                    D_800E71C8[i] = result[i];
                }
            }
            return 2;
        }
        if (D_800D3A14[D_800D396D] != 0) {
            D_800D3C2C[0] = 3;
            if (D_800E71C8 != 0) {
                for (i = 0; i < 8; i++) {
                    D_800E71C8[i] = result[i];
                }
            }
            return 1;
        }
        D_800D3C2C[0] = 2;
        if (D_800E71C8 != 0) {
            for (i = 0; i < 8; i++) {
                D_800E71C8[i] = result[i];
            }
        }
        return 2;

    case 4:
        D_800D3C2C[2] = 4;
        D_800D3C2C[1] = D_800D3C2C[2];
        if (D_800E71D8 != 0) {
            for (i = 0; i < 8; i++) {
                D_800E71D8[i] = result[i];
            }
        }
        if (D_800E71D0 != 0) {
            for (i = 0; i < 8; i++) {
                D_800E71D0[i] = result[i];
            }
        }
        return 4;

    case 5:
        D_800D3C2C[1] = 5;
        D_800D3C2C[0] = D_800D3C2C[1];
        if (D_800E71C8 != 0) {
            for (i = 0; i < 8; i++) {
                D_800E71C8[i] = result[i];
            }
        }
        if (D_800E71D0 != 0) {
            for (i = 0; i < 8; i++) {
                D_800E71D0[i] = result[i];
            }
        }
        return 6;

    default:
        func_800B8D80(D_8001531C);
        func_800AD698(D_80015330, intr);
        return 0;
    }
}
