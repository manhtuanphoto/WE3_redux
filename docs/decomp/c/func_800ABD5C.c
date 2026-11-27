/*
 * func_800ABD5C
 *
 * Purpose:
 *   BGM / sound-track switcher handling both sequence/SPU tracks (ID < 0x200)
 *   and CD-XA audio tracks (ID >= 0x200). Resets master volume via
 *   func_800BC718(0x7F, 0x7F), stops the previous track if switching types or
 *   SEQ tracks, updates D_800E6B76, and either kicks SPU voices (ID >= 0x400)
 *   or sets CD-XA state D_800E6B4C = 8 (for 0x200 <= ID < 0x400).
 *
 * Arguments:
 *   track_id (int / unsigned short): target track identifier
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800E6B76 (unsigned short, R/W): current/previous track ID
 *   D_800E6B4C (signed short, R/W): CD-XA playback state (set to 0 on stop, 8 on start)
 *   D_800E6B82 (unsigned short, R): active SEQ slot index (stride 32 in D_800CE845)
 *   D_800CE845 (unsigned char[], R): SEQ channel/ID table (entry at idx << 5)
 *   D_800E6BD8 (unsigned char, W): cleared to 0
 *   D_800E6B16 (unsigned short, W): cleared to 0
 */

extern unsigned short D_800E6B76;
extern signed short D_800E6B4C;
extern unsigned short D_800E6B82;
extern unsigned char D_800CE845[];
extern unsigned char D_800E6BD8;
extern unsigned short D_800E6B16;

extern void func_800BC718(unsigned int left, unsigned int right);
extern void func_800A95E8(void);
extern void func_800BCBF8(unsigned int seq_id);
extern void func_800BDC78(unsigned int ch);

void func_800ABD5C(int track_id)
{
    unsigned short prev;
    int same_track = 0;

    func_800BC718(0x7Fu, 0x7Fu);

    prev = D_800E6B76;
    if (prev != 0) {
        same_track = (prev == (unsigned int)track_id) ? 1 : 0;
        if (prev >= 0x200u) {
            if (track_id < 0x200) {
                if (D_800E6B4C >= 3) {
                    D_800E6B4C = 8;
                }
                D_800E6B4C = 0;
                func_800A95E8();
            }
        } else if (!same_track) {
            unsigned int idx = D_800E6B82;
            func_800BCBF8(D_800CE845[idx << 5]);
        }
    }

    D_800E6B76 = (unsigned short)track_id;
    D_800E6BD8 = 0;
    D_800E6B16 = 0;

    if (track_id < 0x200) {
        return;
    }

    if (track_id >= 0x400) {
        func_800BDC78(0x12u);
        func_800BDC78(0x13u);
        func_800BDC78(0x14u);
        func_800BDC78(0x15u);
    } else if (track_id >= 0x300 || !same_track) {
        D_800E6B4C = 8;
    }
}
