/*
 * func_80021F28 - Initialize all 22 player actors from team/stat/formation tables
 *
 * Purpose:
 *   Loops over all 22 players (11 per team) and populates each player actor's
 *   roster slot, formation role, side, attack direction, model/skeleton pointer,
 *   scale (derived from height), max speed, acceleration divisor, stamina/condition,
 *   and active-on-pitch flag.
 *
 * Arguments:
 *   None (void).
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Actor fields read & written:
 *   Scratch 0x2E0     (u8)  - kickoff/home-side XOR flag
 *   Scratch 0x2DD[2]  (u8)  - team index offset per side
 *   Scratch 0x2FD[2]  (u8)  - formation id per side
 *   D_8010865D        (u8)  - selector for model/resource buffers D_801B0000 / D_801C8000
 *   Actor +0x00       (u8)  - active flag (1 if status < 2, else 0)
 *   Actor +0x20       (ptr) - player model/data pointer from D_801B0000 / D_801C8000
 *   Actor +0x2C/30/34 (s32) - x/y/z model scale (fixed-point << 12)
 *   Actor +0x38D      (u8)  - actor id (read)
 *   Actor +0x391      (u8)  - formation role id from D_800C8118
 *   Actor +0x392      (u8)  - team slot (0..10)
 *   Actor +0x394      (u16) - global roster index (slot + team_offset * 22)
 *   Actor +0x398      (u8)  - side (0/1 = func_800526C8(actor_id) ^ 1)
 *   Actor +0x399      (u8)  - attack direction flag (0/1 = player / 11)
 *   Actor +0x39A      (u8)  - result of func_80052654(actor_id)
 *   Actor +0x3B3      (u8)  - 0x30 - D_800C6B54[(D_8010C32A & 0xF) * 2]
 *   Actor +0x3B4      (u8)  - max speed (0x35 + D_800C6AE4[speed_level])
 *   Actor +0x3B5      (u8)  - accel divisor (0x12 = 18)
 *   Actor +0x3B6      (s16) - D_800C6B68[D_8010C32C & 0xF]
 *   Actor +0x3B9      (u8)  - stamina/rating stat*10 + condition*5, clamped to 100
 *   Actor +0x3BA      (u8)  - (condition - 2) + ((D_8010C328 >> 12) & 0xF)
 *
 * Magic numbers:
 *   0x35 (53)         = base max speed
 *   0x12 (18)         = acceleration divisor
 *   100..227          = player height in cm (((D_8010C330 >> 11) & 0x7F) + 100)
 *   180               = reference height (cm) for y-scale normalization
 *   230..170          = body-type scale denominator from jtbl_800106B8 (cases 0..9)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char D_800FF748[];
extern unsigned char D_8010865D;
extern unsigned char D_801B0000[];
extern unsigned char D_801C8000[];
extern unsigned char D_800C8118[];
extern unsigned char D_8010C328[];
extern unsigned char D_8010C32A[];
extern unsigned char D_8010C32C[];
extern unsigned char D_8010C330[];
extern unsigned char D_800EF78A[];
extern unsigned char D_800C6B54[];
extern unsigned char D_800C6AE4[];
extern unsigned char D_800C6B68[];

extern unsigned int func_800526C8(unsigned int actor_id);
extern unsigned int func_80052654(unsigned int actor_id);

void func_80021F28(void)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *team_bufs[2];
    unsigned char *actor = D_800FF748 + 1000;
    int i;
    unsigned short xz_denom;

    team_bufs[D_8010865D] = D_801B0000;
    team_bufs[D_8010865D ^ 1] = D_801C8000;

    for (i = 0; i < 22; i++, actor += 1000) {
        int half_team = i / 11;
        unsigned int logical_side = (B(scratch, 0x2E0) ^ (unsigned int)half_team) & 0xFF;
        int slot_in_team = i - half_team * 11;
        unsigned int roster_slot = B(D_800FF748, 0x8E9C + logical_side * 22 + slot_in_team);
        unsigned int team_offset = B(scratch, 0x2DD + logical_side);
        unsigned int formation = B(scratch, 0x2FD + logical_side);
        unsigned int actor_id;
        unsigned int side;
        unsigned int stat_off;
        unsigned int cond_off;
        unsigned char cond_raw;
        int cond_adj;
        unsigned int body_type;
        int height_scaled;
        int xz_scale;
        int y_scale;
        unsigned int stamina_val;

        B(actor, 0x392) = (unsigned char)roster_slot;
        H(actor, 0x394) = (unsigned short)(roster_slot + team_offset * 22);
        B(actor, 0x391) = B(D_800C8118, formation * 11 + slot_in_team);

        actor_id = B(actor, 0x38D);
        side = func_800526C8(actor_id) ^ 1;
        B(actor, 0x398) = (unsigned char)side;
        B(actor, 0x39A) = (unsigned char)func_80052654(actor_id);
        B(actor, 0x399) = (unsigned char)half_team;

        W(actor, 0x20) = W(team_bufs[logical_side], B(actor, 0x392) * 4);

        stat_off = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        cond_off = B(actor, 0x392) * 4 + B(actor, 0x398) * 88;
        cond_raw = B(D_800EF78A, cond_off);
        cond_adj = (int)cond_raw - 2;

        B(actor, 0x3B3) = (unsigned char)(0x30 - B(D_800C6B54, (H(D_8010C32A, stat_off) & 0xF) * 2));

        stat_off = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        B(actor, 0x3B5) = 0x12;
        B(actor, 0x3B4) = (unsigned char)(B(D_800C6AE4, (W(D_8010C328, stat_off) >> 20) & 0xF) + 0x35);

        stat_off = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        S(actor, 0x3B6) = (signed short)B(D_800C6B68, W(D_8010C32C, stat_off) & 0xF);

        stat_off = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        B(actor, 0x3BA) = (unsigned char)(cond_adj + ((W(D_8010C328, stat_off) >> 12) & 0xF));

        stat_off = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        body_type = (W(D_8010C328, stat_off) >> 4) & 0xF;
        switch (body_type) {
        case 0: xz_denom = 0xE6; break;
        case 1: xz_denom = 0xDC; break;
        case 2: xz_denom = 0xD2; break;
        case 3: xz_denom = 0xC8; break;
        case 4: xz_denom = 0xC3; break;
        case 5: xz_denom = 0xBE; break;
        case 6: xz_denom = 0xB9; break;
        case 7: xz_denom = 0xB4; break;
        case 8: xz_denom = 0xAF; break;
        case 9: xz_denom = 0xAA; break;
        }

        stat_off = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        height_scaled = (int)(((W(D_8010C330, stat_off) >> 11) & 0x7F) + 100) << 12;
        xz_scale = height_scaled / (int)(xz_denom + 10);
        SW(actor, 0x34) = xz_scale;
        SW(actor, 0x2C) = xz_scale;

        stat_off = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        height_scaled = (int)(((W(D_8010C330, stat_off) >> 11) & 0x7F) + 100) << 12;
        y_scale = height_scaled / 180;
        SW(actor, 0x30) = y_scale;

        stat_off = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        stamina_val = (W(D_8010C328, stat_off) >> 28) * 10 + (unsigned int)((signed short)cond_adj * 5);
        B(actor, 0x3B9) = (unsigned char)stamina_val;
        if (B(actor, 0x3B9) >= 101) {
            B(actor, 0x3B9) = 100;
        }

        if (B(D_800FF748, 0x88A5 + B(actor, 0x398) * 22 + B(actor, 0x392)) < 2) {
            B(actor, 0) = 1;
        } else {
            B(actor, 0) = 0;
        }
    }
}
