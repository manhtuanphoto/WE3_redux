/*
 * func_800C0AB8 - Select active VAB bank and program entry for sound playback
 *
 * Purpose:
 *   Validates that `vab_id` is < 16, active (D_8010A3F0[vab_id] == 1), and
 *   `prog_id` < D_800F106C. On success, records `vab_id`, `prog_id`, and the
 *   tone count byte from `D_800EF8B8[vab_id][prog_id * 16 + 8]` into
 *   D_8010A3C8+1/+6/+7, copies VAB table pointers into D_800F20B0, D_800F20AC,
 *   and D_800F1098, and returns 0. Returns -1 on invalid arguments.
 *
 * Arguments:
 *   vab_id (signed short):  VAB sound bank slot index (0..15)
 *   prog_id (signed short): program index (< D_800F106C)
 *
 * Return value:
 *   0 on success; -1 if slot/program validation fails
 *
 * Globals read & written:
 *   D_8010A3F0 (u8[16], R): VAB slot active flags (must be 1)
 *   D_800F106C (s16, R): max program count
 *   D_800EF940 (u32[16], R): per-VAB table pointer copied to D_800F20AC
 *   D_800EF8B8 (u8 *[16], R): per-VAB program table (16-byte entries) copied to D_800F1098
 *   D_800EF988 (u32[16], R): per-VAB table pointer copied to D_800F20B0
 *   D_8010A3C8 (u8[], W): +0x01 = vab_id, +0x06 = prog_id, +0x07 = prog_entry[+0x08]
 *   D_800F20B0, D_800F20AC, D_800F1098 (W): active VAB pointers
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))

extern unsigned char D_8010A3F0[];
extern signed short D_800F106C;
extern unsigned int D_800EF940[];
extern unsigned char *D_800EF8B8[];
extern unsigned int D_800EF988[];
extern unsigned char D_8010A3C8[];
extern unsigned int D_800F20B0;
extern unsigned int D_800F20AC;
extern unsigned char *D_800F1098;

signed int func_800C0AB8(signed short vab_id, signed short prog_id)
{
    signed int slot = (signed int)vab_id;
    signed int prog = (signed int)prog_id;
    unsigned char *prog_table;
    unsigned int val_940;
    unsigned int val_988;

    if (((unsigned int)vab_id & 0xFFFFu) >= 0x10u) {
        return -1;
    }
    if (D_8010A3F0[slot] != 1u) {
        return -1;
    }
    if (prog >= (signed int)D_800F106C) {
        return -1;
    }

    val_940 = D_800EF940[slot];
    prog_table = D_800EF8B8[slot];
    val_988 = D_800EF988[slot];

    D_8010A3C8[1] = (unsigned char)vab_id;
    D_8010A3C8[6] = (unsigned char)prog_id;
    D_800F20B0 = val_988;
    D_800F20AC = val_940;
    D_800F1098 = prog_table;
    D_8010A3C8[7] = B(prog_table + (prog * 16), 8);

    return 0;
}
