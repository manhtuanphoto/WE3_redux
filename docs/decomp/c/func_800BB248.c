/*
 * func_800BB248
 *
 * Purpose:
 *   Per-tick sound/sequence driver dispatcher (PsyQ libsnd SsSeqCalledTbyT
 *   equivalent). Guards against re-entrancy via D_800F1014, ticks global driver
 *   state via func_800BE4A8, and iterates active sequences (D_800FF72C bitmask,
 *   up to D_8010CD40) and tracks (up to D_8010CD48, 0xB0-byte track structs in
 *   D_8010C2A8[seq]) dispatching flag bits 0x01..0x80 at +0x98.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800F1014 (u32) re-entrancy lock flag,
 *   D_8010CD40 (s16) number of sequence slots,
 *   D_8010CD48 (s16) number of tracks per sequence,
 *   D_800FF72C (u32) active sequence bitmask,
 *   D_8010C2A8 (u8 *[]) array of sequence track tables (stride 0xB0 per track, flags u32 at +0x98)
 *
 * Global fields written:
 *   D_800F1014 (u32) set to 1 while running, cleared to 0 on exit,
 *   track+0x98 (u32) cleared to 0 when bit 0x04 is handled
 */

#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned int   D_800F1014;
extern signed short   D_8010CD40;
extern signed short   D_8010CD48;
extern unsigned int   D_800FF72C;
extern unsigned char *D_8010C2A8[];

extern void func_800BE4A8(void);
extern void func_800BBC08(int seq, int track);
extern void func_800BB4B8(int seq, int track);
extern void func_800BB7C8(int seq, int track);
extern void func_800BD108(int seq, int track);
extern void func_800BBB68(int seq, int track);
extern void func_800BC4D8(int seq, int track);
extern void func_800BCA78(int seq, int track);

void func_800BB248(void)
{
    int seq;

    if (D_800F1014 == 1) {
        return;
    }

    D_800F1014 = 1;
    func_800BE4A8();

    for (seq = 0; seq < (int)D_8010CD40; seq++) {
        int track;
        unsigned int trackOff;

        if ((D_800FF72C & (1U << seq)) == 0) {
            continue;
        }
        if (D_8010CD48 <= 0) {
            continue;
        }

        trackOff = 0;
        for (track = 0; track < (int)D_8010CD48; track++, trackOff += 0xB0) {
            unsigned char *base = D_8010C2A8[seq];

            if ((W(base + trackOff, 0x98) & 0x01U) != 0) {
                func_800BBC08((signed short)seq, (signed short)track);

                if ((W(D_8010C2A8[seq] + trackOff, 0x98) & 0x10U) != 0) {
                    func_800BB4B8((signed short)seq, (signed short)track);
                }
                if ((W(D_8010C2A8[seq] + trackOff, 0x98) & 0x20U) != 0) {
                    func_800BB7C8((signed short)seq, (signed short)track);
                }
                if ((W(D_8010C2A8[seq] + trackOff, 0x98) & 0x40U) != 0) {
                    func_800BD108((signed short)seq, (signed short)track);
                }
                if ((W(D_8010C2A8[seq] + trackOff, 0x98) & 0x80U) != 0) {
                    func_800BD108((signed short)seq, (signed short)track);
                }
            }

            if ((W(D_8010C2A8[seq] + trackOff, 0x98) & 0x02U) != 0) {
                func_800BBB68((signed short)seq, (signed short)track);
            }
            if ((W(D_8010C2A8[seq] + trackOff, 0x98) & 0x08U) != 0) {
                func_800BC4D8((signed short)seq, (signed short)track);
            }
            if ((W(D_8010C2A8[seq] + trackOff, 0x98) & 0x04U) != 0) {
                func_800BCA78((signed short)seq, (signed short)track);
                W(D_8010C2A8[seq] + trackOff, 0x98) = 0;
            }
        }
    }

    D_800F1014 = 0;
}
