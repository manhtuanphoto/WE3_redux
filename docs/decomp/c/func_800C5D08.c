/*
 * func_800C5D08 (and trailing unlabeled helper in func_800C5D08.s)
 *
 * Purpose:
 *   func_800C5D08 writes a 128-byte PS1 Memory Card frame with an XOR checksum
 *   at byte 0x7F and verifies it via read-back, retrying up to 8 times.
 *   The trailing unlabeled function in the same assembly unit formats a PS1
 *   Memory Card by writing 15 directory frames (sectors 1..15), 20 broken-sector
 *   frames (sectors 16..35), and the "MC" header frame (sector 0).
 *
 * Arguments (func_800C5D08):
 *   port:   (int) memory card port/channel selector (port >> 4 passed to status poll)
 *   sector: (int) 128-byte sector index on the memory card
 *   buf:    (unsigned char*) 128-byte data buffer (byte 0x7F overwritten with XOR sum)
 *
 * Return value:
 *   1 on successful write + checksum verification, 0 on failure.
 *
 * Magic numbers:
 *   0x7F: 128-byte PS1 memory card frame checksum byte offset (XOR of bytes 0..0x7E)
 *   0x80: PS1 memory card frame size (128 bytes)
 *   0x08: max write+verify retry attempts
 *   0xA0 / 0xFFFF: PS1 memory card directory entry free-block tag and link value
 *   0x4D ('M'), 0x43 ('C'): PS1 memory card header magic bytes at sector 0
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))

extern unsigned char D_8001569C[];
extern unsigned char D_800E7348[];
extern unsigned int  D_800E7528[];
extern unsigned char D_800E7578[];

extern void func_800AD658(void *dst, int len);           /* memset/bzero */
extern void func_800AD698(const void *msg);              /* debug print / log */
extern void func_800C59A8(int port);
/* UNCLEAR: In assembly, func_800C59A8(port) is called immediately before
 * func_800C5998 and func_800C6058 without reloading $a0; $a0 is assumed
 * preserved or unused by the callees as written in the compact asm. */
extern int  func_800C5998(int port, int sector, unsigned char *buf);
extern int  func_800C6058(int port, int sector, unsigned char *buf);
extern int  func_800C6068(int channel);

int func_800C5D08(int port, int sector, unsigned char *buf) {
    unsigned char verify_buf[0x80];
    unsigned char xor_sum;
    int retry;
    int i;

    xor_sum = 0;
    for (i = 0; i < 0x7F; i++) {
        xor_sum ^= buf[i];
    }
    buf[0x7F] = xor_sum;

    for (retry = 0; retry < 8; retry++) {
        func_800C59A8(port);
        if (func_800C5998(port, sector, buf) != 1) {
            return 0;
        }

        while ((func_800C6068(port >> 4) & 1) == 0) {
        }

        func_800AD658(verify_buf, 0x80);
        func_800C59A8(port);
        if (func_800C6058(port, sector, verify_buf) == 1) {
            while ((func_800C6068(port >> 4) & 1) == 0) {
            }
        } else {
            func_800AD698(D_8001569C);
        }

        xor_sum = 0;
        for (i = 0; i < 0x7F; i++) {
            xor_sum ^= verify_buf[i];
        }
        if (buf[0x7F] == xor_sum) {
            return 1;
        }
    }

    return 0;
}

/* UNCLEAR: Unlabeled second function appended inside func_800C5D08.s (no glabel).
 * Formats a PS1 Memory Card on `port` by writing directory blocks 1..15,
 * broken-sector table blocks 16..35, and header block 0 ("MC"). */
int func_800C5D08_format_card(int port) {
    int i;
    int j;

    for (i = 0; i < 15; i++) {
        unsigned char *dir_entry = D_800E7348 + (i << 5);

        func_800AD658(D_800E7578, 0x80);
        func_800AD658(dir_entry, 0x20);
        W(dir_entry, 0x00) = 0xA0;
        W(dir_entry, 0x04) = 0;
        H(dir_entry, 0x08) = 0xFFFF;

        for (j = 0; j < 0x20; j++) {
            D_800E7578[j] = dir_entry[j];
        }

        if (func_800C5D08(port, i + 1, D_800E7578) != 1) {
            return 0;
        }
    }

    for (i = 0; i < 20; i++) {
        D_800E7528[i] = 0xFFFFFFFFU;
        func_800AD658(D_800E7578, 0x80);
        W(D_800E7578, 0x00) = D_800E7528[i];

        if (func_800C5D08(port, i + 0x10, D_800E7578) != 1) {
            return 0;
        }
    }

    func_800AD658(D_800E7578, 0x80);
    D_800E7578[0] = 'M';
    D_800E7578[1] = 'C';
    return (func_800C5D08(port, 0, D_800E7578) == 1) ? 1 : 0;
}
