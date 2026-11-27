/*
 * func_800AC084 - Initialize CD-XA track fade volumes and playback state 4
 *
 * Purpose:
 *   Looks up track fade parameters from 6-byte table D_800CE9D8[(track_id - 0x200) * 6],
 *   clears stall/retry counters (D_800E6B00, D_800E6BA4), sets initial L/R fade
 *   volumes (D_800E6AD6, D_800E6ADC), calls func_800A920C(15), sets CD state
 *   D_800E6B4C = 4, clears 16 halfwords backwards from D_8010CD7E to D_8010CD60,
 *   and applies master-scaled stereo volume via func_800BCC58.
 *
 * Arguments:
 *   track_id (signed int): track ID (0x200-based index into D_800CE9D8)
 *
 * Return value:
 *   None (void)
 *
 * Globals read & written:
 *   D_800CE9D8 (u8[], R): 6-byte track descriptor table (+0/+1 = L/R fade volumes)
 *   D_800E6B00 (u8, W): stall/retry counter cleared to 0
 *   D_800E6BA4 (s16, W): startup counter cleared to 0
 *   D_800E6AD6 / D_800E6ADC (s16, R/W): L/R fade volumes
 *   D_800E6B6C (s16, R): master volume scale (product >> 7)
 *   D_800E6B4C (s16, W): CD playback state set to 4
 *   D_8010CD7E (u16[], W): 16-halfword buffer cleared backwards (D_8010CD7E down to D_8010CD60)
 */

extern unsigned char D_800CE9D8[];
extern unsigned char D_800E6B00;
extern signed short D_800E6BA4;
extern signed short D_800E6AD6;
extern signed short D_800E6ADC;
extern signed short D_800E6B6C;
extern signed short D_800E6B4C;
extern unsigned short D_8010CD7E[];

extern void func_800A920C(int arg0);
extern void func_800BCC58(int arg0, signed int vol_l, signed int vol_r);

void func_800AC084(signed int track_id)
{
    signed int offset = (track_id - 0x200) * 6;
    signed int master;
    signed int vol_l;
    signed int vol_r;
    int i;

    D_800E6B00 = 0;
    D_800E6BA4 = 0;

    D_800E6AD6 = (signed short)D_800CE9D8[offset];
    D_800E6ADC = (signed short)D_800CE9D8[offset + 1];

    /* Note: assembly relies on func_800A920C preserving $a0 = 0xF for the loop below */
    func_800A920C(0xF);

    D_800E6B4C = 4;
    for (i = 0; i < 16; i++) {
        D_8010CD7E[-i] = 0;
    }

    master = (signed int)D_800E6B6C;
    vol_l = (signed int)D_800E6AD6 * master;
    if (vol_l < 0) {
        vol_l += 0x7F;
    }
    vol_l = (signed short)(vol_l >> 7);

    vol_r = (signed int)D_800E6ADC * master;
    if (vol_r < 0) {
        vol_r += 0x7F;
    }
    vol_r = (signed short)(vol_r >> 7);

    func_800BCC58(0, vol_l, vol_r);
}
