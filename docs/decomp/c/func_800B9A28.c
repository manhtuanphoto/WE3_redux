/*
 * func_800B9A28
 *
 * Purpose:
 *   CD-ROM / STR video sector streaming handler (typically invoked on sector
 *   ready / ring-buffer update). Reads or DMAs the 32-byte sector header and
 *   2016-byte (0x1F8 words) payload into the circular frame buffer (either from
 *   a RAM preloaded buffer D_8010CD4C via func_800BA344 or via hardware DMA
 *   func_800BA370), validates STR magic (0x0160), channel, chunk index, and
 *   frame number, handles ring-buffer wrap/overflow and end-of-frame signaling
 *   (D_800FF680 = 1).
 *
 * Arguments:
 *   None (void).
 *
 * Return value:
 *   None (void).
 *
 * Actor fields read/written:
 *   None.
 *
 * Scratchpad fields read/written:
 *   None.
 *
 * Globals read/written:
 *   D_800FF680 (u32 frame-complete flag; early-exits if 1, set to 1 on last chunk)
 *   D_800E81D4 (u32 streaming/mode flag)
 *   D_800E81D8 (u32 skip-header-drain flag)
 *   D_800D3D44 (volatile u32 * hardware status register; bit 24 = 0x01000000 busy)
 *   D_800D3D54 (volatile u32 * hardware DMA/CD status register; bit 24 busy)
 *   D_800D3D24, D_800D3D2C, D_800D3D30, D_800D3D34, D_800D3D38 (hardware reg ptrs)
 *   D_800D3D6C (u32 diagnostic/status code 1..10)
 *   D_800EF91C (u32 set to 1 on busy abort)
 *   D_8010CD4C (u32 RAM stream source base, 0 = use CD DMA)
 *   D_8010A59C (u32 sector index into D_8010CD4C, 2048 bytes/sector)
 *   D_8010CD8C (u8 * ring-buffer header array base, 32 bytes/slot)
 *   D_8010C1C0 (u32 write sector index)
 *   D_8010C1C4 (u32 frame-start sector index)
 *   D_8010CE68 (u32 ring-buffer capacity in sectors)
 *   D_800E7208 (u8 * current 32-byte STR sector header ptr)
 *   D_8010CD88 (u8 * current 2016-byte sector payload ptr)
 *   D_8010CD84 (u32 seek/resync pending flag)
 *   D_8010CD58 (u32 max frame number limit, 0 = unlimited)
 *   D_800F0F6C (u32 expected resync frame number)
 *   D_800F0F68 (u32 next channel id)
 *   D_800F3560 (u32 active STR channel id)
 *   D_800E81CC (s16/u16 expected chunk index within frame)
 *   D_800E7654 (u32 active frame number)
 *   D_800EF8B4 (void (*)(void) callback ptr invoked on frame limit / buffer full)
 *
 * Magic numbers / unit conversions:
 *   0x0160 = PS1 STR video sector magic
 *   0x1F8  = 504 words = 2016 bytes (CD sector 2048 bytes minus 32-byte header)
 *   0x20943 / 0x20843 / 0x21020843, 0x1323 / 0x1325, 0x11000000 / 0x11400100 =
 *     PS1 CD-ROM / DMA register control constants
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned int D_800FF680;
extern unsigned int D_800E81D4;
extern unsigned int D_800E81D8;
extern volatile unsigned int *D_800D3D44;
extern volatile unsigned int *D_800D3D54;
extern volatile unsigned char *D_800D3D24;
extern volatile unsigned char *D_800D3D2C;
extern volatile unsigned char *D_800D3D30;
extern volatile unsigned int *D_800D3D34;
extern volatile unsigned int *D_800D3D38;
extern unsigned int D_800D3D6C;
extern unsigned int D_800EF91C;
extern unsigned int D_8010CD4C;
extern unsigned int D_8010A59C;
extern unsigned char *D_8010CD8C;
extern unsigned int D_8010C1C0;
extern unsigned int D_8010C1C4;
extern unsigned int D_8010CE68;
extern unsigned char *D_800E7208;
extern unsigned char *D_8010CD88;
extern unsigned int D_8010CD84;
extern unsigned int D_8010CD58;
extern unsigned int D_800F0F6C;
extern unsigned int D_800F0F68;
extern unsigned int D_800F3560;
extern signed short D_800E81CC;
extern unsigned int D_800E7654;
extern unsigned int D_800EF8B4;

extern int func_800B6FEC(int mode, void *out);
extern void func_800BA344(unsigned char *dst, unsigned int src, unsigned int words, int flag);
extern void func_800BA370(int ch, unsigned char *dst, unsigned int arg2, unsigned int words,
                          unsigned int chcr, unsigned int arg5, unsigned int arg6);
extern void func_800B9908(unsigned int start, unsigned int count);
extern void func_800B96D8(void);

void func_800B9A28(void)
{
    unsigned char status_buf[8];
    unsigned char hdr_tail[8];
    unsigned int dma_chcr;
    unsigned char *hdr;
    unsigned int i;

    if (D_800FF680 == 1) {
        return;
    }

    if (D_800E81D4 != 0 && (*D_800D3D44 & 0x01000000U) != 0) {
        D_800EF91C = 1;
        if (D_8010CD4C != 0) {
            D_8010A59C++;
        }
        D_800D3D6C = 1;
        return;
    }

    if (func_800B6FEC(1, status_buf) == 5) {
        return;
    }

    if ((status_buf[0] & 4) != 0) {
        D_800D3D6C = 3;
        return;
    }

    hdr = D_8010CD8C + (D_8010C1C0 << 5);
    D_800E7208 = hdr;
    if (H(hdr, 0x00) != 0) {
        if (D_8010CD4C != 0) {
            D_8010A59C++;
        }
        D_800D3D6C = 4;
        return;
    }

    *D_800D3D24 = 0;
    *D_800D3D30 = 0;
    *D_800D3D24 = 0;
    *D_800D3D30 = 0x80;
    *D_800D3D34 = 0x20943;
    *D_800D3D38 = 0x1323;

    if (D_800E81D8 == 0) {
        for (i = 0; i < 4; i++) {
            hdr_tail[i] = *D_800D3D2C;
        }
        for (i = 0; i < 8; i++) {
            (void)*D_800D3D2C;
        }
    }

    if (D_8010CD4C != 0) {
        func_800BA344(D_800E7208, D_8010CD4C + (D_8010A59C << 11), 8, 0);
    } else {
        func_800BA370(3, D_800E7208, 0, 8, 0x11000000, 0, 0);
    }

    while ((*D_800D3D54 & 0x01000000U) != 0) {
    }

    W(D_800E7208, 0x1C) = W(hdr_tail, 0);
    *D_800D3D34 = 0x20843;
    *D_800D3D38 = 0x1325;

    if (D_8010CD84 == 1 && D_800F0F6C != 0) {
        if (D_800F0F6C == H(D_800E7208, 0x08)) {
            H(D_800E7208, 0x00) = 0;
            D_8010CD84 = 0;
        } else {
            H(D_800E7208, 0x00) = 0;
            if (D_8010CD4C != 0) {
                D_8010A59C++;
            }
            return;
        }
    }

    hdr = D_800E7208;
    if (H(hdr, 0x00) != 0x160 || ((H(hdr, 0x02) >> 10) & 0x1F) != D_800F3560) {
        if (D_8010CD4C != 0) {
            D_8010A59C = 0;
        }
        D_800D3D6C = 5;
        H(D_800E7208, 0x00) = 0;
        return;
    }

    if (D_800E81CC != (int)H(hdr, 0x04) ||
        (D_800E7654 != 0 && D_800E7654 != H(hdr, 0x08))) {
        D_800E7654 = 0;
        D_800E81CC = 0;
        func_800B9908(D_8010C1C4, D_8010C1C0 - D_8010C1C4);
        D_8010C1C0 = D_8010C1C4;
        H(D_800E7208, 0x00) = 0;
        if (D_8010CD4C != 0) {
            D_8010A59C++;
        }
        D_800D3D6C = 6;
        return;
    }

    if (H(D_800E7208, 0x04) == 0) {
        D_800E81CC = 0;
        D_800E7654 = H(D_800E7208, 0x08);
        if (D_8010CD58 != 0 && D_800E7654 >= D_8010CD58) {
            D_800E7654 = 0;
            D_800E81CC = 0;
            func_800B9908(D_8010C1C4, D_8010C1C0 - D_8010C1C4);
            D_8010C1C0 = D_8010C1C4;
            H(D_800E7208, 0x00) = 0;
            D_8010CD84 = 1;
            if (D_800EF8B4 != 0) {
                ((void (*)(void))D_800EF8B4)();
            }
            if (D_8010CD4C != 0) {
                D_8010A59C++;
            }
            D_800D3D6C = 7;
            return;
        }

        if ((unsigned int)((D_8010CE68 - D_8010C1C0) - 1) < (unsigned int)H(D_800E7208, 0x06)) {
            if (D_8010CD58 == 0) {
                H(D_800E7208, 0x00) = 1;
                D_8010CD84 = 1;
                if (D_800EF8B4 != 0) {
                    ((void (*)(void))D_800EF8B4)();
                }
                if (D_8010CD4C != 0) {
                    D_8010A59C++;
                }
                D_800D3D6C = 8;
                return;
            }
            if (S(D_8010CD8C, 0x00) != 0) {
                H(D_800E7208, 0x00) = 0;
                if (D_8010CD4C != 0) {
                    D_8010A59C++;
                }
                D_800D3D6C = 9;
                return;
            }
            H(D_800E7208, 0x00) = 1;
            D_8010C1C0 = 0;
            for (i = 0; i < 8; i++) {
                W(D_8010CD8C, i * 4) = W(D_800E7208, i * 4);
            }
            D_800E7208 = D_8010CD8C;
        }
        D_8010C1C4 = D_8010C1C0;
    }

    D_800D3D6C = 0xA;
    D_800E81CC = (signed short)(D_800E81CC + 1);
    D_8010CD88 = D_8010CD8C + (D_8010CE68 << 5) + (((D_8010C1C0 << 6) - D_8010C1C0) << 5);

    if (D_800E81D4 != 0) {
        dma_chcr = 0x11000000U;
        *D_800D3D34 = 0x20943;
        *D_800D3D38 = 0x1323;
    } else {
        dma_chcr = 0x11400100U;
        *D_800D3D34 = 0x21020843;
    }

    if (H(D_800E7208, 0x06) - 1 == H(D_800E7208, 0x04)) {
        D_800FF680 = 1;
        if (D_8010CD4C != 0) {
            func_800BA344(D_8010CD88, D_8010CD4C + (D_8010A59C << 11) + 0x20, 0x1F8, 1);
            D_8010A59C++;
        } else {
            func_800BA370(3, D_8010CD88, 0, 0x1F8, dma_chcr, 1, 0);
        }
        D_800E81CC = 0;
        D_800E7654 = 0;
        D_800F3560 = D_800F0F68;
    } else {
        if (D_8010CD4C != 0) {
            func_800BA344(D_8010CD88, D_8010CD4C + (D_8010A59C << 11) + 0x20, 0x1F8, 0);
            D_8010A59C++;
        } else {
            func_800BA370(3, D_8010CD88, 0, 0x1F8, dma_chcr, 0, 0);
        }
    }

    *D_800D3D38 = 0x1325;
    H(D_800E7208, 0x00) = 3;
    D_8010C1C0++;

    if (D_8010CD4C != 0 && D_800FF680 != 0) {
        func_800B96D8();
    }
}
