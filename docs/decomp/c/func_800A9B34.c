/*
 * func_800A9B34
 *
 * Purpose:
 *   Commentary / XA-ADPCM / SPU voice streaming and sound-bank loader state
 *   machine. Dispatches on state D_800E6B84 (0x70..0x87) to resolve sound/phrase
 *   IDs from descriptor tables, seek and stream sectors from CD into RAM buffer
 *   0x801E3000, upload ADPCM data to SPU SRAM (0x1010 / 0x11010 / 0x21010 /
 *   0x31010), configure SPU voice attributes/volumes, and copy phrase tables
 *   into 0x801E2C00.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   int - 0 on early exit or invalid/out-of-range state; updated D_800E6B84 on
 *   most state-advancing paths. (Callers such as func_800AA7C4 ignore the
 *   return value; paths jumping to the reset tail leave the prior expression
 *   value in $v0.)
 *
 * Globals / memory fields read:
 *   - D_800E6BD8 (u8): disable/busy flag (1 = abort early)
 *   - D_800E6BCC (u32): requested sound/commentary ID
 *   - D_800E6B74 (s16): secondary enable flag when ID < 0x7D0
 *   - D_8010A5D7 (u8): streaming/pause flag (1 = accumulate D_8010CDDC into D_800E6B78)
 *   - D_8010CDDC (s32): frame delta added to D_800E6B78
 *   - D_800E6B84 (u16): current state (0x70..0x87)
 *   - D_800E6ACC (u32): CD sector offset
 *   - D_800E6B04 (s32): sector/size count
 *   - D_800E6AF0 (u8[...]): CD status/result buffer (bit 0x10 = shell open / error)
 *   - D_800E6B54 (s16): CD ready retry counter (timeout at 0x15 = 21 frames)
 *   - D_800E6A9C (s16): read retry counter (max 3 retries before abort to 0x7F)
 *   - D_800E6A90 (u16): queued next sound ID
 *   - D_800E6B0C (u32): wait/lock flag in state 0x7F
 *   - D_800E6BD2 (u16), D_800E6B6C (u16), D_800E6AA0 (s16): SPU/volume parameters
 *   - D_800E6BC4 (s16), D_800E6BC6 (s16): 44-entry table indices for state 0x7A
 *   - D_800CE268 / D_800CE26A / D_800CE26C: phrase/CD descriptor tables
 *   - D_800CE758 / D_800CE778 / D_800CE798: SPU volume lookup tables
 *
 * Globals / memory fields written:
 *   - D_800E6B78 (s32): accumulated timer (reset to 0 on completion)
 *   - D_800E6AD8 (u8 *): set to 0x801E3000 (CD/ADPCM staging buffer)
 *   - D_800E6B5C (u8 *): set to 0x801E1800 (sound descriptor table base)
 *   - D_800E6B7C (u8 *): set to 0x801E3000 (state 0x7A)
 *   - D_800E6B3C (u8 *): set to 0x801E2C00 (state 0x7A destination)
 *   - D_800E6B84 (u16): next state (0 to reset, 0x7F on CD error/shell open)
 *   - D_800E6AE4 (u32): cumulative requested ID checksum/counter
 *   - D_800E6ACC (u32), D_800E6B04 (s32), D_800E6BD0 (s16), D_800E6B94 (u32)
 *   - D_800E6A8C (u32): SPU voice/channel bitmask (bits 22 and 23: 0x400000 / 0x800000)
 *   - D_800E6B2A (u16), D_800F0F2C (u32), D_800F0F30/D_800F0F32/D_800F0F3C (u16)
 *
 * Magic numbers:
 *   - 0x15 (21) frames -> 0.70 s CD-ready wait timeout (at 30 Hz)
 *   - 0x40 (64) bytes -> zeroed header block before SPU DMA upload of 0x10000 (64 KiB)
 *   - 0x20 (32) sectors -> max single-chunk CD read; larger transfers split at 0x20
 *   - 0x2C (44) entries -> 4-byte records copied per bank in state 0x7A (0xB0 = 176 bytes)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800CE268[];
extern unsigned char D_800CE26A[];
extern unsigned char D_800CE26C[];
extern unsigned char D_800CE758[];
extern unsigned char D_800CE778[];
extern unsigned char D_800CE798[];

extern unsigned char D_800E6A8C[];
extern unsigned char D_800E6A90[];
extern unsigned char D_800E6A9C[];
extern unsigned char D_800E6AA0[];
extern unsigned char D_800E6ACC[];
extern unsigned char D_800E6AD8[];
extern unsigned char D_800E6AE4[];
extern unsigned char D_800E6AF0[];
extern unsigned char D_800E6B04[];
extern unsigned char D_800E6B0C[];
extern unsigned char D_800E6B2A[];
extern unsigned char D_800E6B3C[];
extern unsigned char D_800E6B54[];
extern unsigned char D_800E6B5C[];
extern unsigned char D_800E6B6C[];
extern unsigned char D_800E6B74[];
extern unsigned char D_800E6B78[];
extern unsigned char D_800E6B7C[];
extern unsigned char D_800E6B84[];
extern unsigned char D_800E6B94[];
extern unsigned char D_800E6BC4[];
extern unsigned char D_800E6BC6[];
extern unsigned char D_800E6BCC[];
extern unsigned char D_800E6BD0[];
extern unsigned char D_800E6BD2[];
extern unsigned char D_800E6BD8[];

extern unsigned char D_800F0F2C[];
extern unsigned char D_800F0F30[];
extern unsigned char D_800F0F32[];
extern unsigned char D_800F0F3C[];

extern unsigned char D_8010A5D7[];
extern unsigned char D_8010CDDC[];

extern void func_800AAA10(void);
extern void func_800AAC24(void);
extern void func_800AAC48(void);
extern void func_800AAC6C(void);
extern void func_800AAC90(void);
extern void func_800AB620(int id);
extern int func_800B6FCC(int mode, unsigned char *result);
extern int func_800B7034(unsigned int cmd, unsigned char *param, unsigned char *result);
extern unsigned char *func_800B7494(int sector, unsigned char *loc);
extern int func_800B9348(int sectors, unsigned char *buf, int mode);
extern int func_800B9448(int mode, unsigned char *result);
extern void func_800C3D58(unsigned char *buf, int size);
extern void func_800C3DB8(int spu_addr);
extern int func_800C3ED8(int mode);
extern void func_800C44A8(unsigned char *attr);

int func_800A9B34(void)
{
    unsigned char cd_loc[16];
    unsigned int sound_id;
    unsigned char *table_base;
    unsigned char *entry;
    unsigned char *adpcm_buf;
    int status;
    int i;
    int ret;
    unsigned short vol;

    if (B(D_800E6BD8, 0) == 1) {
        return 0;
    }

    if (W(D_800E6BCC, 0) < 0x7D0 && S(D_800E6B74, 0) == 0) {
        H(D_800E6B84, 0) = 0;
        return 0;
    }

    if (B(D_8010A5D7, 0) == 1) {
        SW(D_800E6B78, 0) += SW(D_8010CDDC, 0);
    }

    W(D_800E6AD8, 0) = 0x801E3000U;
    W(D_800E6B5C, 0) = 0x801E1800U;

    switch (H(D_800E6B84, 0)) {
    case 0x70:
        sound_id = W(D_800E6BCC, 0);
        H(D_800E6B84, 0) = 0x70;
        W(D_800E6AE4, 0) += sound_id;

        if (sound_id < 0xBB9U) {
            if (sound_id < 0x7D1U) {
                if (sound_id < 0x500U) {
                    table_base = (unsigned char *)W(D_800E6B5C, 0);
                    entry = table_base + sound_id * 4;
                    W(D_800E6ACC, 0) = H(entry, 0) + 0xC738U;
                    W(D_800E6B04, 0) = B(entry, 2);
                } else {
                    table_base = (unsigned char *)W(D_800E6B5C, 0);
                    entry = table_base + sound_id * 4;
                    W(D_800E6ACC, 0) = H(entry, 0);
                    W(D_800E6B04, 0) = B(entry, 2);
                    W(D_800E6ACC, 0) += 0x4E20U;
                }
            } else {
                unsigned int idx = sound_id - 0x7D0U;
                W(D_800E6ACC, 0) = H(D_800CE26C, idx * 4) + 0x4E20U;
                W(D_800E6B04, 0) = B(D_800CE26C, idx * 4 + 2);
            }
        } else if (sound_id < 0xBBAU) {
            W(D_800E6ACC, 0) = H(D_800CE268, 0);
            W(D_800E6B04, 0) = B(D_800CE26A, 0);
        }

        if (SW(D_800E6B04, 0) < 2) {
            H(D_800E6B84, 0) = 0;
            return 1;
        }

        if (func_800B6FCC(1, D_800E6AF0) == 2) {
            if (B(D_800E6AF0, 0) & 0x10) {
                H(D_800E6B84, 0) = 0x7F;
                return 0x7F;
            }
            func_800B7494(SW(D_800E6ACC, 0), cd_loc);
            func_800B7034(2, cd_loc, 0); /* CdlSetloc */
            H(D_800E6B84, 0) += 1;
            return H(D_800E6B84, 0);
        } else {
            H(D_800E6B54, 0) += 1;
            if (S(D_800E6B54, 0) >= 0x15) {
                H(D_800E6B84, 0) = 0x7F;
                return 0x7F;
            }
        }
        break;

    case 0x71:
        if (func_800B6FCC(1, D_800E6AF0) != 2) {
            return 0;
        }
        if (B(D_800E6AF0, 0) & 0x10) {
            H(D_800E6B84, 0) = 0x7F;
            return 0x7F;
        }

        if (SW(D_800E6B04, 0) < 0x21) {
            do {
                ret = func_800B9348(SW(D_800E6B04, 0), (unsigned char *)W(D_800E6AD8, 0), 0x80);
            } while (ret == 0);
        } else {
            do {
                ret = func_800B9348(0x20, (unsigned char *)W(D_800E6AD8, 0), 0x80);
            } while (ret == 0);
        }

        sound_id = W(D_800E6BCC, 0);
        if (sound_id < 0x7D1U) {
            table_base = (unsigned char *)W(D_800E6B5C, 0);
            H(D_800E6BD0, 0) = B(table_base, sound_id * 4 + 2);
        } else {
            H(D_800E6BD0, 0) = B(D_800CE26C, (sound_id - 0x7D0U) * 4 + 2);
        }
        H(D_800E6B84, 0) += 1;
        W(D_800E6B94, 0) = 0;
        return H(D_800E6B84, 0);

    case 0x72:
        sound_id = W(D_800E6BCC, 0);
        if (sound_id - 0x7D2U < 0x3E6U) {
            func_800AAA10();
        }
        if (func_800B6FCC(1, D_800E6AF0) == 2 && (B(D_800E6AF0, 0) & 0x10)) {
            H(D_800E6B84, 0) = 0x7F;
            return 0x7F;
        }

        status = func_800B9448(1, D_800E6AF0);
        H(D_800E6BD0, 0) = (unsigned short)status;
        if (status == -1) {
            if (W(D_800E6BCC, 0) < 0x7D1U) {
                H(D_800E6B84, 0) = 0x7F;
            } else {
                H(D_800E6A9C, 0) += 1;
                if (S(D_800E6A9C, 0) < 4) {
                    H(D_800E6B84, 0) = 0x70;
                } else {
                    H(D_800E6B84, 0) = 0x7F;
                    H(D_800E6A9C, 0) = 0;
                }
            }
        }
        if (status == 0) {
            sound_id = W(D_800E6BCC, 0);
            H(D_800E6A9C, 0) = 0;
            if (sound_id >= 0xBB9U) {
                H(D_800E6B84, 0) = 0x7A;
            } else if (sound_id < 0x7D1U) {
                H(D_800E6B84, 0) += 1;
                return H(D_800E6B84, 0);
            } else {
                H(D_800E6B84, 0) = 0x80;
            }
        }
        break;

    case 0x73:
        if (B(D_8010A5D7, 0) == 1) {
            return 0;
        }
        func_800AAC48();
        H(D_800E6B84, 0) += 1;
        return H(D_800E6B84, 0);

    case 0x74:
        if (B(D_8010A5D7, 0) == 1) {
            return 0;
        }
        func_800C3DB8(0x21010);
        adpcm_buf = (unsigned char *)W(D_800E6AD8, 0);
        for (i = 0; i < 0x40; i++) {
            B(adpcm_buf, i) = 0;
        }
        func_800C3D58((unsigned char *)W(D_800E6AD8, 0), 0x10000);
        H(D_800E6B84, 0) += 1;
        return H(D_800E6B84, 0);

    case 0x75:
        if (func_800C3ED8(0) != 1) {
            return 0;
        }
        if (B(D_8010A5D7, 0) != 1) {
            table_base = (unsigned char *)W(D_800E6B5C, 0);
            if (B(table_base, W(D_800E6BCC, 0) * 4 + 3) == 1) {
                W(D_800E6A8C, 0) &= 0x400000U;
            } else {
                W(D_800E6A8C, 0) |= 0x800000U;
            }
            func_800AAC24();
            W(D_800E6B78, 0) = 0;
            if (SW(D_800E6B04, 0) >= 0x21) {
                H(D_800E6B84, 0) = 0x82;
                return 0x82;
            }
            H(D_800E6B84, 0) = 0;
        }
        if (SW(D_800E6B04, 0) >= 0x10 && H(D_800E6A90, 0) != 0) {
            W(D_800E6BCC, 0) = H(D_800E6A90, 0);
            H(D_800E6B84, 0) = 0x70;
            H(D_800E6B54, 0) = 0;
            H(D_800E6A90, 0) = 0;
        }
        break;

    case 0x7A:
    {
        int bank0 = S(D_800E6BC4, 0) * 0x2C;
        int bank1 = S(D_800E6BC6, 0) * 0x2C;
        W(D_800E6B7C, 0) = 0x801E3000U;
        W(D_800E6B3C, 0) = 0x801E2C00U;

        for (i = 0; i < 0x2C; i++) {
            unsigned char *src = (unsigned char *)W(D_800E6B7C, 0) + (bank0 + i) * 4;
            unsigned char *dst = (unsigned char *)W(D_800E6B3C, 0) + i * 4;
            H(dst, 0) = H(src, 0);
            B(dst, 2) = B(src, 2);
            B(dst, 3) = 1;
        }
        for (i = 0; i < 0x2C; i++) {
            unsigned char *src = (unsigned char *)W(D_800E6B7C, 0) + (bank1 + i) * 4;
            unsigned char *dst = (unsigned char *)W(D_800E6B3C, 0) + i * 4;
            H(dst, 0xB0) = H(src, 0);
            B(dst, 0xB2) = B(src, 2);
            B(dst, 0xB3) = 1;
        }
        H(D_800E6B84, 0) = 0;
        break;
    }

    case 0x7F:
        if (W(D_800E6B0C, 0) == 0) {
            func_800B7034(1, 0, D_800E6AF0); /* CdlNop */
            if ((B(D_800E6AF0, 0) & 0x10) == 0) {
                H(D_800E6B84, 0) = 0;
            }
        }
        break;

    case 0x80:
        H(D_800E6B2A, 0) = H(D_800E6BD2, 0);
        func_800AAC90();
        H(D_800E6B84, 0) += 1;
        return H(D_800E6B84, 0);

    case 0x81:
        adpcm_buf = (unsigned char *)W(D_800E6AD8, 0);
        if (B(adpcm_buf, 0x41) != 6 || B(adpcm_buf, 0x841) != 2 || B(adpcm_buf, 0x1041) != 2) {
            H(D_800E6B84, 0) = 0;
            return 0;
        }
        func_800C3DB8(0x1010);
        for (i = 0; i < 0x40; i++) {
            B((unsigned char *)W(D_800E6AD8, 0), i) = 0;
        }
        func_800C3D58((unsigned char *)W(D_800E6AD8, 0), 0x10000);
        if (SW(D_800E6B04, 0) >= 0x21) {
            H(D_800E6B84, 0) += 1;
            return H(D_800E6B84, 0);
        }
        H(D_800E6B84, 0) = 0x86;
        break;

    case 0x82:
        if (func_800B6FCC(1, D_800E6AF0) == 2) {
            if (B(D_800E6AF0, 0) & 0x10) {
                H(D_800E6B84, 0) = 0x7F;
                return 0x7F;
            }
            func_800B7494(SW(D_800E6ACC, 0) + 0x20, cd_loc);
            func_800B7034(2, cd_loc, 0);
            H(D_800E6B84, 0) += 1;
            return H(D_800E6B84, 0);
        } else {
            H(D_800E6B54, 0) += 1;
            if (S(D_800E6B54, 0) >= 0x15) {
                H(D_800E6B84, 0) = 0x7F;
                return 0x7F;
            }
        }
        break;

    case 0x83:
        if (func_800B6FCC(1, D_800E6AF0) != 2) {
            return 0;
        }
        if (B(D_800E6AF0, 0) & 0x10) {
            H(D_800E6B84, 0) = 0x7F;
            return 0x7F;
        }
        do {
            ret = func_800B9348(SW(D_800E6B04, 0) - 0x20, (unsigned char *)W(D_800E6AD8, 0), 0x80);
        } while (ret == 0);

        sound_id = W(D_800E6BCC, 0);
        if (sound_id < 0x7D1U) {
            table_base = (unsigned char *)W(D_800E6B5C, 0);
            H(D_800E6BD0, 0) = B(table_base, sound_id * 4 + 2);
        } else {
            H(D_800E6BD0, 0) = B(D_800CE26C, (sound_id - 0x7D0U) * 4 + 2);
        }
        H(D_800E6B84, 0) += 1;
        W(D_800E6B94, 0) = 0;
        return H(D_800E6B84, 0);

    case 0x84:
        if (func_800B6FCC(1, D_800E6AF0) == 2 && (B(D_800E6AF0, 0) & 0x10)) {
            H(D_800E6B84, 0) = 0x7F;
            return 0x7F;
        }
        status = func_800B9448(1, D_800E6AF0);
        H(D_800E6BD0, 0) = (unsigned short)status;
        if (status == -1) {
            if (W(D_800E6BCC, 0) < 0x7D1U) {
                H(D_800E6B84, 0) = 0x7F;
            } else {
                H(D_800E6A9C, 0) += 1;
                if (S(D_800E6A9C, 0) < 4) {
                    H(D_800E6B84, 0) = 0x70;
                } else {
                    H(D_800E6B84, 0) = 0x7F;
                    H(D_800E6A9C, 0) = 0;
                }
            }
        }
        if (status == 0) {
            H(D_800E6A9C, 0) = 0;
            if (W(D_800E6BCC, 0) < 0xBB9U) {
                H(D_800E6B84, 0) += 1;
                return H(D_800E6B84, 0);
            }
            H(D_800E6B84, 0) = 0x7A;
        }
        break;

    case 0x85:
        if (W(D_800E6BCC, 0) < 0x7D1U) {
            func_800C3DB8(0x31010);
            func_800C3D58((unsigned char *)W(D_800E6AD8, 0), 0x10000);
            H(D_800E6B84, 0) = 0;
        } else {
            H(D_800E6B84, 0) += 1;
            func_800C3DB8(0x11010);
            func_800C3D58((unsigned char *)W(D_800E6AD8, 0), 0x10000);
        }
        break;

    case 0x86:
        if (func_800C3ED8(0) != 1) {
            return 0;
        }
        W(D_800F0F2C, 0) = 0x13;
        switch (B(D_800CE26C, (W(D_800E6BCC, 0) - 0x7D0U) * 4 + 3)) {
        case 0:
            H(D_800F0F3C, 0) = 0xB9C;
            vol = H(D_800E6B6C, 0) * 96;
            H(D_800F0F30, 0) = vol;
            H(D_800F0F32, 0) = vol;
            func_800C44A8(D_800F0F3C - 0x14);
            func_800AB620(0x520);
            func_800AB620(0x513);
            break;

        case 1:
            break;

        case 2:
            H(D_800F0F3C, 0) = 0x5CE;
            vol = H(D_800CE758, S(D_800E6AA0, 0) * 2) << 7;
            H(D_800F0F30, 0) = vol;
            H(D_800F0F32, 0) = vol;
            func_800C44A8(D_800F0F3C - 0x14);
            break;

        case 3:
            H(D_800F0F3C, 0) = 0x5CE;
            vol = H(D_800CE778, S(D_800E6AA0, 0) * 2) << 7;
            H(D_800F0F30, 0) = vol;
            H(D_800F0F32, 0) = vol;
            func_800C44A8(D_800F0F3C - 0x14);
            break;

        case 4:
            H(D_800F0F3C, 0) = 0x5CE;
            vol = H(D_800CE798, S(D_800E6AA0, 0) * 2) << 7;
            H(D_800F0F30, 0) = vol;
            H(D_800F0F32, 0) = vol;
            func_800C44A8(D_800F0F3C - 0x14);
            break;

        case 5:
            H(D_800F0F3C, 0) = 0x400;
            vol = H(D_800E6B6C, 0) * 96;
            H(D_800F0F30, 0) = vol;
            H(D_800F0F32, 0) = vol;
            func_800C44A8(D_800F0F3C - 0x14);
            break;

        case 6:
            H(D_800F0F3C, 0) = 0x800;
            vol = H(D_800E6B6C, 0) * 96;
            H(D_800F0F30, 0) = vol;
            H(D_800F0F32, 0) = vol;
            func_800C44A8(D_800F0F3C - 0x14);
            break;

        case 7:
            H(D_800F0F3C, 0) = 0xA20;
            vol = H(D_800E6B6C, 0) * 96;
            H(D_800F0F30, 0) = vol;
            H(D_800F0F32, 0) = vol;
            func_800C44A8(D_800F0F3C - 0x14);
            func_800AB620(0x520);
            func_800AB620(0x513);
            break;
        }

        if (B(D_800CE26C, (W(D_800E6BCC, 0) - 0x7D0U) * 4 + 3) != 0) {
            W(D_800E6A8C, 0) |= 0x400000U;
        } else {
            W(D_800E6A8C, 0) &= 0x800000U;
        }
        H(D_800E6B84, 0) += 1;
        return H(D_800E6B84, 0);

    case 0x87:
        if (B(D_800CE26C, (W(D_800E6BCC, 0) - 0x7D0U) * 4 + 3) == 0) {
            func_800AB620(0x520);
            func_800AB620(0x513);
        }
        func_800AAC6C();
        W(D_800E6B78, 0) = 0;
        H(D_800E6B84, 0) = 0;
        break;

    default:
        return 0;
    }

    return 0;
}
