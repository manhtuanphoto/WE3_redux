/*
 * func_8004F82C
 *
 * Purpose:
 *   Updates 10 animated UI/HUD elements (indices 0..9) stored at stride 0x28
 *   bytes in the workspace tail of D_800FF748 (+0x61D0 base: active byte at
 *   +0x04 / +0x5DC4+0x410, Y position s16 at +0x10 / +0x5DD0+0x410). Each
 *   active element follows a 3-state vertical slide/hold machine driven by
 *   D_800E6C28[i] and timer D_800E6C48[i]:
 *     - state 0: counts down timer; on <= 0 moves Y by -0x40 until reaching
 *       target D_800C7BC4[D_800E6C18][i], then clamps Y, sets timer = 0x1C (28),
 *       and advances to state 1.
 *     - state 1: counts down timer; on <= 0 advances to state 2.
 *     - state 2: moves Y by -0x40 per frame until Y < -0xC0, then deactivates.
 *
 * Arguments:
 *   void
 *
 * Return value:
 *   void
 *
 * Global fields read / written:
 *   D_800FF748 (u8[]): element workspace (+0x410 + 0x5DC4 + i*0x28 active u8,
 *                      +0x410 + 0x5DD0 + i*0x28 Y s16)
 *   D_800E6C28 (s16[10]): per-element state (0, 1, 2)
 *   D_800E6C48 (u16[10]): per-element delay/hold frame timer
 *   D_800E6C18 (s16): layout preset index (20-byte stride in D_800C7BC4)
 *   D_800C7BC4 (s16[][10]): target Y positions per preset
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define H(p, o) (*(unsigned short *)((p) + (o)))
#define S(p, o) (*(signed short *)((p) + (o)))

extern unsigned char D_800FF748[];
extern signed short D_800E6C28[];
extern unsigned short D_800E6C48[];
extern signed short D_800E6C18;
extern unsigned char D_800C7BC4[];

void func_8004F82C(void)
{
    signed int i;
    signed int elemOff = 0x410;

    for (i = 0; i < 10; i++, elemOff += 0x28) {
        unsigned char *entry = D_800FF748 + elemOff;
        if (B(entry, 0x5DC4) == 0) {
            continue;
        }

        switch (D_800E6C28[i]) {
        case 0: {
            signed short timer = (signed short)(D_800E6C48[i] - 1);
            D_800E6C48[i] = (unsigned short)timer;
            if (timer <= 0) {
                signed short y = (signed short)(H(entry, 0x5DD0) - 0x40);
                signed short targetY = S(D_800C7BC4, ((signed int)D_800E6C18 * 20) + (i * 2));
                H(entry, 0x5DD0) = (unsigned short)y;
                if (y < targetY) {
                    H(entry, 0x5DD0) = (unsigned short)targetY;
                    D_800E6C48[i] = 0x1C;
                    D_800E6C28[i]++;
                }
            }
            break;
        }

        case 1: {
            signed short timer = (signed short)(D_800E6C48[i] - 1);
            D_800E6C48[i] = (unsigned short)timer;
            if (timer <= 0) {
                D_800E6C28[i]++;
            }
            break;
        }

        case 2: {
            signed short y = (signed short)(H(entry, 0x5DD0) - 0x40);
            H(entry, 0x5DD0) = (unsigned short)y;
            if (y < -0xC0) {
                B(entry, 0x5DC4) = 0;
            }
            break;
        }

        default:
            break;
        }
    }
}
