/*
 * func_80017684
 *
 * Purpose:
 *   Commentary / CD-XA audio playback state machine driven by D_800F106E (states 0..6).
 *   Looks up clip metadata in 12-byte table D_800C6098[clip_id] and steps through
 *   CD sync, sector seek, XA parameter setup, playback completion wait, and cooldown.
 *
 * Arguments:
 *   clip_id (unsigned short): index into 12-byte commentary entry table D_800C6098
 *
 * Return value:
 *   1 if state == 0, or if clip_id resolves to a negative first word (SPU/ADPCM path),
 *     or when state 6 finishes its 2-frame cooldown;
 *   0 while seek/playback is in progress or state >= 7.
 *
 * Global fields read:
 *   D_800F106E (u8) state (0..6)
 *   D_800C6098 (12-byte entries: +0x00 s32, +0x04 u32, +0x08 u32)
 *   D_800E6C00 / D_800E6C04 / D_800E6C08 (u32) active clip fields
 *   D_800E6C0C (u8) retry/cooldown frame counter (16 frames in state 2; 2 frames in state 6)
 *   D_800E6BF8 (u8[]) CD location buffer
 *
 * Global fields written:
 *   D_800F106E (u8), D_800F3564 (u8), D_800E6BFC (s32),
 *   D_800E6C00 (u32), D_800E6C04 (u32), D_800E6C08 (u32), D_800E6C0C (u8)
 */

#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char D_800F106E;
extern unsigned char D_800F3564;
extern unsigned char D_800C6098[];
extern unsigned int  D_800E6C00;
extern unsigned int  D_800E6C04;
extern unsigned int  D_800E6C08;
extern unsigned char D_800E6C0C;
extern unsigned char D_800E6BF8[];
extern signed int    D_800E6BFC;

extern void func_800AD648(signed int a0, unsigned int a1, unsigned int a2);
extern unsigned int func_800B7170(unsigned int cmd, unsigned char *buf);
extern unsigned int func_800B6FCC(unsigned int a0, unsigned int a1);
extern void func_800B7494(unsigned int param, unsigned char *buf);
extern unsigned int func_800B9348(unsigned int a0, unsigned int a1, unsigned int vol);
extern signed int func_800B9448(unsigned int a0, unsigned int a1);

unsigned int func_80017684(unsigned short clip_id)
{
    unsigned char state = D_800F106E;

    if (state >= 7) {
        return 0;
    }

    switch (state) {
    case 0:
        return 1;

    case 1: {
        unsigned char *entry = D_800C6098 + ((unsigned int)(clip_id & 0xFFFF) * 12);
        signed int p0 = SW(entry, 0);
        unsigned int p1 = W(entry, 4);
        unsigned int p2 = W(entry, 8);

        D_800F3564 = 0;
        D_800E6C00 = (unsigned int)p0;
        D_800E6C04 = p1;
        D_800E6C08 = p2;

        if (p0 < 0) {
            func_800AD648(p0, p1, p2 << 11);
            return 1;
        }

        D_800E6C0C = 0;
        D_800F106E = 2;
        return 0;
    }

    case 2:
        if (func_800B7170(9, 0) != 0) {
            D_800F106E = 3;
            return 0;
        }
        D_800E6C0C = (unsigned char)(D_800E6C0C + 1);
        if (D_800E6C0C < 0x10) {
            return 0;
        }
        D_800F106E = 1;
        return 0;

    case 3:
        if (func_800B6FCC(1, 0) != 2) {
            return 0;
        }
        func_800B7494(D_800E6C00, D_800E6BF8);
        if (func_800B7170(2, D_800E6BF8) == 0) {
            D_800F106E = 2;
            return 0;
        }
        D_800F106E = 4;
        return 0;

    case 4: {
        unsigned int st = func_800B6FCC(1, 0);
        if (st != 2) {
            if (st == 5) {
                D_800F106E = 2;
            }
            return 0;
        }
        if (func_800B9348(D_800E6C08, D_800E6C04, 0x80) == 0) {
            return 0;
        }
        D_800F106E = 5;
        return 0;
    }

    case 5: {
        signed int res = func_800B9448(1, 0);
        D_800E6BFC = res;
        if (res == 0) {
            D_800F106E = 6;
            D_800E6C0C = 2;
            return 0;
        }
        if (res == -1) {
            D_800F106E = 2;
        }
        return 0;
    }

    case 6:
        D_800E6C0C = (unsigned char)(D_800E6C0C - 1);
        if (D_800E6C0C != 0) {
            return 0;
        }
        D_800F106E = 0;
        return 1;
    }

    return 0;
}
