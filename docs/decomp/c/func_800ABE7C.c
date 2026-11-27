/*
 * func_800ABE7C
 *
 * Purpose:
 *   Audio/commentary cue dispatcher. Ignores calls when D_800E6B76 == 0x204.
 *   For cue IDs >= 0x800 (unless D_800E6B76 is 0x14E8 or 0x201), resets
 *   D_800E6B4C and D_800E6B76 and calls func_800AAB74().
 *   For cue IDs < 0x5DC, looks up an 8-byte entry in D_800CE868 indexed by
 *   (cue_id - 0x500), stops any previous active cue on that channel (unless 0x270F),
 *   starts playback via func_800BCEF8, and records the active cue in D_800EF898.
 *
 * Arguments:
 *   cue_id - audio/commentary cue identifier
 *
 * Return value:
 *   None (void)
 *
 * Global fields read / written:
 *   D_800E6B76 (u16, gp-relative)
 *   D_800E6B4C (u16, gp-relative)
 *   D_800E6B34 (u16/s16, gp-relative) = cue_id - 0x500
 *   D_800CE868 (8-byte cue descriptor table: s16 unk0, s16 unk2, u8 unk4, u8 unk5, u8 unk7)
 *   D_800EF898 (s16 array indexed by channel unk7)
 */

extern unsigned short D_800E6B76;
extern unsigned short D_800E6B4C;
extern unsigned short D_800E6B34;

struct CueDesc {
    short unk0;
    short unk2;
    unsigned char unk4;
    unsigned char unk5;
    unsigned char unk6;
    unsigned char channel; /* +0x7: D_800CE86C + 3 */
};

extern struct CueDesc D_800CE868[];
extern short D_800EF898[];

extern void func_800AAB74(void);
extern void func_800BCEB8(short arg0, short arg1);
extern void func_800BCEF8(short arg0, short arg1, unsigned char arg2, unsigned char arg3);

void func_800ABE7C(int cue_id)
{
    unsigned short state = D_800E6B76;
    unsigned short idx;
    unsigned char ch;

    if (state == 0x204) {
        return;
    }

    D_800E6B34 = (unsigned short)(cue_id - 0x500);

    if (cue_id >= 0x800) {
        if (state == 0x14E8 || state == 0x201) {
            return;
        }
        D_800E6B4C = 0;
        D_800E6B76 = 0;
        func_800AAB74();
        return;
    }

    if (cue_id >= 0x5DC) {
        return;
    }

    idx = D_800E6B34;
    ch = D_800CE868[idx].channel;

    if (ch != 0) {
        short prev = D_800EF898[ch];
        if (prev != 0x270F) {
            func_800BCEB8(D_800CE868[prev].unk0, D_800CE868[prev].unk2);
        }
        idx = D_800E6B34;
        func_800BCEF8(
            D_800CE868[idx].unk0,
            D_800CE868[idx].unk2,
            D_800CE868[idx].unk4,
            D_800CE868[idx].unk5
        );
        idx = D_800E6B34;
        D_800EF898[D_800CE868[idx].channel] = (short)idx;
    } else {
        func_800BCEB8(D_800CE868[idx].unk0, D_800CE868[idx].unk2);
        idx = D_800E6B34;
        func_800BCEF8(
            D_800CE868[idx].unk0,
            D_800CE868[idx].unk2,
            D_800CE868[idx].unk4,
            D_800CE868[idx].unk5
        );
    }
}
