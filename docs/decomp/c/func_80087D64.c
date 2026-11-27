/*
 * func_80087D64 - Initialize replay playback camera, ring buffer index, and celebration mode
 *
 * Purpose:
 *   Sets up replay camera parameters, locates the next valid frame slot in the
 *   120-entry replay ring buffer (D_800FB5D0, 0x7FFF = empty), configures replay
 *   type (D_800EF9C8 cases 0..4) including optional goal-scorer focus and SFX,
 *   and starts the replay timer.
 *
 * Arguments:
 *   None (void).
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Global fields read & written:
 *   Scratch 0x1E8 (u8)  - set to 1
 *   Scratch 0x288 (s16) - current replay ring buffer index (0..119)
 *   Scratch 0x2AD (u8)  - kicker/scorer actor id
 *   Scratch 0x2AE (u8)  - replay focus actor id
 *   Scratch 0x2C0 (u8)  - cleared to 0
 *   Scratch 0x2E2 (u8)  - match/goal mode flag
 *   Scratch 0x2E7 (u8)  - camera angle
 *   Scratch 0x2EA (u16) - camera distance/zoom (0x2000 or 0x1200)
 *   Scratch 0x2F2 (u8)  - replay camera mode (0, 3, 4, 8, or 0xC)
 *   D_800EF9C8    (u8)  - replay type (0..4)
 *   D_800F0EE0    (u16) - active replay ring buffer slot
 *   D_8010C2A0    (u16) - previous valid replay ring buffer slot
 *   D_800FB5D0[]  (s16) - 120-entry replay frame/x table (0x7FFF = sentinel)
 *   D_800FB5A8    (u8)  - replay timer (0xF0 = 240 frames = 8 s at 30 Hz)
 *   D_800E6D68    (u8)  - set to 1
 *   D_8010A323    (u8)  - flag checked in case 0 (D_800FF748 + 0xABDB)
 *
 * Magic numbers:
 *   120           = replay ring buffer capacity (0x77 = 119 max index)
 *   0x7FFF        = empty ring-buffer slot sentinel
 *   0x22E3 (8931) = 35.724 m (x-distance threshold choosing camera mode 4 vs 8)
 *   0xF0   (240)  = 8.0 s at 30 Hz
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char D_800FF748[];
extern unsigned char D_800EF9C8;
extern unsigned short D_800F0EE0;
extern unsigned short D_8010C2A0;
extern signed short D_800FB5D0[];
extern unsigned char D_800FB5A8;
extern unsigned char D_800E6D68;

extern void func_8001DD50(int);
extern void func_80098428(void);
extern void func_8004FD14(int);
extern void func_80096050(void);
extern int func_8001E6B8(int);
extern void func_8001CEE0(int, int, int, int, int, int, int, int);
extern unsigned char func_8005CE30(unsigned char);
extern void func_800AB620(int);
extern void func_800573EC(void);
extern void func_8001722C(int);
extern void func_800171DC(void);

void func_80087D64(void)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    int rand_cam;
    signed short start_idx;
    unsigned short prev_idx;

    func_8001DD50(0xC);
    func_80098428();
    func_8004FD14(1);
    B(scratch, 0x2C0) = 0;
    func_80096050();

    rand_cam = func_8001E6B8(4);
    B(scratch, 0x2E7) = (unsigned char)(rand_cam << 6);
    H(scratch, 0x2EA) = 0x2000;
    D_800F0EE0 = H(scratch, 0x288);

    func_8001CEE0(2, 0x3D, 0x60000000, 0, 0, 6, 0, 1);
    if (D_800EF9C8 == 0) {
        func_8001CEE0(1, 0x3E, 0, 0, 0, 6, 0, 1);
        B(scratch, 0x2AE) = 0;
    }

    if (D_800FB5D0[D_800F0EE0] == 0x7FFF) {
        start_idx = S(scratch, 0x288);
        do {
            D_800F0EE0 = (unsigned short)((D_800F0EE0 + 1) % 120);
            if ((int)D_800F0EE0 == (int)start_idx) {
                func_8001722C(3);
                return;
            }
        } while (D_800FB5D0[D_800F0EE0] == 0x7FFF);
    }

    H(scratch, 0x288) = D_800F0EE0;
    prev_idx = (D_800F0EE0 != 0) ? (unsigned short)(D_800F0EE0 - 1) : 0x77;
    D_8010C2A0 = prev_idx;

    while (D_800FB5D0[D_8010C2A0] == 0x7FFF) {
        unsigned short cur = D_8010C2A0;
        D_8010C2A0 = (cur != 0) ? (unsigned short)(cur - 1) : 0x77;
    }

    switch (D_800EF9C8) {
    case 0:
        if (B(D_800FF748, 0xABDB) == 0 && func_8001E6B8(8) == 0) {
            unsigned char actor_id = B(scratch, 0x2AD);
            unsigned char *actor = D_800FF748 + (actor_id % 24) * 1000;
            unsigned char mode = B(scratch, 0x2E2);
            unsigned char *focus_actor;

            if (mode == 0 || (unsigned int)(mode - 2) < 2) {
                if (func_8001E6B8(3) != 0 && B(actor, 0x398) == 1) {
                    B(scratch, 0x2AE) = func_8005CE30(B(actor, 0x38D));
                } else {
                    B(scratch, 0x2AE) = B(scratch, 0x2AD);
                }
            } else {
                B(scratch, 0x2AE) = actor_id;
            }

            focus_actor = D_800FF748 + (B(scratch, 0x2AE) % 24) * 1000;
            H(scratch, 0x2EA) = 0x1200;
            B(scratch, 0x2E7) = (unsigned char)(B(focus_actor, 0x399) << 7);
        } else {
            B(scratch, 0x2AE) = 0;
        }
        B(scratch, 0x2F2) = 0;
        func_800AB620(0x13B9);
        func_800AB620(0x13B9);
        func_800AB620(0x520);
        func_800573EC();
        break;

    case 1:
        B(scratch, 0x2F2) = 3;
        break;

    case 2:
    case 3:
        if (func_8001E6B8(2) != 0) {
            int val = D_800FB5D0[D_800F0EE0];
            if (val < 0) {
                val = -val;
            }
            B(scratch, 0x2F2) = (val < 0x22E3) ? 8 : 4;
        } else {
            B(scratch, 0x2F2) = 8;
        }
        break;

    case 4:
        B(scratch, 0x2F2) = 0xC;
        H(scratch, 0x288) = (unsigned short)((S(scratch, 0x288) + 0x30) % 120);
        break;
    }

    D_800FB5A8 = 0xF0;
    D_800E6D68 = 1;
    B(scratch, 0x1E8) = 1;
    func_800171DC();
}
