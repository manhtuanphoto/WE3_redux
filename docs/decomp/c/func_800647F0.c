/*
 * func_800647F0
 *
 * Purpose:
 *   AI decision step for an actor (pass / playmaking state machine driven by
 *   D_8010C1ED). In state 0, optionally aborts near touchline (|z| >= 0x1601)
 *   via a random check using D_800C84F0[Scratchpad[0x2DD + team] / 10], evaluates
 *   a candidate action via func_800653D4, and if facing forward within +/-0x27
 *   angle units searches 10 teammates for a forward pass target (within +/-0x2F
 *   angle units) via func_800666DC, otherwise triggers func_8005E09C. In state 1,
 *   advances D_8010C1EC to 2 and sets D_8010C1ED based on |actor->z|.
 *
 * Arguments:
 *   actor: (unsigned char*) pointer to player entity struct
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x01A (u16[2]) per-team pass parameter passed to func_800666DC (R)
 *     +0x2DD (u8[2])  per-team rating/stat divided by 10 to index D_800C84F0 (R)
 *   D_8010C1ED (u8): state variable (0, 1, or 0x20) (R/W)
 *   D_8010C1EC (u8): set to 2 in state 1 (W)
 *   D_8010C1D8: AI state struct (+0x03 u8, +0x14 u8, +0x15 u8) (R/W)
 *   D_800C84F0: lookup table mapping 0 -> 6, 10 -> 10, 20 -> 8 for func_8001E6B8
 *   D_800C84A8, D_800C84B0: side-specific tables passed to func_800653D4
 *   D_800FF748: player array base (stride 0x3E8)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_8010C1ED;
extern unsigned char D_8010C1EC;
extern unsigned char D_8010C1D8[];
extern unsigned char D_800C84F0[];
extern unsigned char D_800C84A8[];
extern unsigned char D_800C84B0[];
extern unsigned char D_800FF748[];

extern int func_8001E6B8(int n);
extern void func_80064B44(unsigned char *actor);
extern unsigned int func_800653D4(unsigned char *tbl, int arg1, int arg2);
extern unsigned int func_8005CE14(unsigned int player_id);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern void func_800666DC(unsigned char *actor, unsigned int target_id, int arg2, unsigned int arg3);
extern void func_8005E09C(unsigned char *actor, unsigned int action, int arg2);

void func_800647F0(unsigned char *actor) {
    unsigned char state = D_8010C1ED;

    if (state == 0) {
        unsigned char idx = (unsigned char)(B(Scratchpad, 0x2DD + B(actor, 0x398)) / 10U);
        unsigned char tbl_val = D_800C84F0[idx];
        int rand_bound = 0; /* UNCLEAR: left uninitialized in asm if tbl_val not in {0, 10, 20} */
        unsigned char cand;

        if (tbl_val == 0) {
            rand_bound = 6;
        } else if (tbl_val == 10) {
            rand_bound = 10;
        } else if (tbl_val == 20) {
            rand_bound = 8;
        }

        if (func_8001E6B8(rand_bound & 0xFF) == 0) {
            signed short az = S(actor, 0x06);
            if (((az >= 0) ? az : -az) >= 0x1601) {
                D_8010C1D8[0x15] = 1;
                return;
            }
        }

        func_80064B44(actor);
        cand = (unsigned char)func_800653D4(
            (B(actor, 0x399) != 0) ? D_800C84B0 : D_800C84A8, 3, 2);

        if (B(actor, 0x396) != 2) {
            unsigned char atk_ang = (unsigned char)(B(actor, 0x399) << 7);
            unsigned char facing = B(actor, 0x3AA);
            unsigned char d = (unsigned char)(atk_ang - facing);
            int aligned = (d < 0x81) ? (d < 0x28) : (((unsigned char)(facing - atk_ang)) < 0x28);

            if (aligned) {
                unsigned int base_slot = func_8005CE14(B(actor, 0x38D)) & 0xFF;
                unsigned char *teammate = D_800FF748 + ((base_slot + 10) * 0x3E8);
                int i;

                for (i = 0; i < 10; i++) {
                    unsigned char ang = (unsigned char)func_8005B36C(
                        S(actor, 0x02), S(actor, 0x06),
                        S(teammate, 0x02), S(teammate, 0x06));
                    unsigned char fwd = (unsigned char)(B(actor, 0x399) << 7);
                    unsigned char diff = (unsigned char)(ang - fwd);
                    int in_cone;

                    if (diff < 0x81) {
                        in_cone = (diff < 0x30);
                    } else {
                        in_cone = (((unsigned char)(fwd - ang)) < 0x30);
                    }

                    if (in_cone) {
                        func_800666DC(actor, B(teammate, 0x38D), 0xFF,
                                      H(Scratchpad, 0x01A + (B(actor, 0x398) << 1)));
                        return;
                    }

                    teammate -= 0x3E8;
                }

                D_8010C1D8[0x14] = 2;
                D_8010C1D8[0x15] = 0;
                D_8010C1D8[0x03] = (unsigned char)(D_8010C1D8[0x03] - 1);
                return;
            }
        }

        if (cand == 0xFF) {
            D_8010C1D8[0x15] = 1;
        } else {
            func_8005E09C(actor, cand, 0);
        }
    } else if (state == 1) {
        signed short az = S(actor, 0x06);
        int abs_z = (az >= 0) ? az : -az;
        D_8010C1EC = 2;
        D_8010C1ED = (abs_z >= 0x1601) ? 0x20 : 0;
    }
}
