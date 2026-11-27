/*
 * func_8001EE84
 *
 * Purpose:
 *   Initializes match-half / extra-time state, clock durations, and all 22
 *   players' stamina values at the start of a half or extra-time period, then
 *   triggers kickoff audio/HUD setup.
 *
 * Arguments:
 *   unused_a0 (int)             - unused ($a0 overwritten before read)
 *   actor     (unsigned char *) - player actor used to look up D_800EF78A
 *                                 team/tactic modifier via +0x392 and +0x398
 *
 * Return value:
 *   None (void)
 *
 * Scratchpad (0x1F800000) / Global fields read & written:
 *   Scratchpad +0x198 (u32) - set to 1
 *   Scratchpad +0x19C (u32) - set to 2
 *   Scratchpad +0x1E8 (u8)  - cleared to 0
 *   Scratchpad +0x1EC (u8)  - cleared to 0
 *   Scratchpad +0x298 (u8)  - match/screen state (3 checked for crowd/whistle SFX)
 *   Scratchpad +0x2A8 (u8)  - last-touch / kickoff count? (0xB or 0x16 from D_8010865D)
 *   Scratchpad +0x2C2 (u8)  - cleared to 0
 *   Scratchpad +0x2C4 (u32) - half timer frames: 9000*(len+1) if period<2, else 3000*(len+1)
 *   Scratchpad +0x2C8 (u32) - set to 0x78 (120 frames = 4 s)
 *   Scratchpad +0x2CE (u8)  - mute/skip-audio flag
 *   Scratchpad +0x2DF (u8)  - period index (0 = 1st half, 1 = 2nd half, >=2 = extra time)
 *   Scratchpad +0x2E1 (u8)  - competition/match mode (4 or 5 skips audio)
 *   D_8010A5A8 +0x00  (u8)  - match half-length setting (0..)
 *   D_8010865D        (u8)  - non-zero -> Scratchpad+0x2A8 = 0x16, else 0xB
 *   D_800EF78A        (u8[])- lookup indexed by (actor+0x392)*4 + (actor+0x398)*88
 *   D_8010C328        (u32[])- player stats table indexed by slot*12 + side*264 (bits [11:8] = stamina)
 *   D_800C6AD8        (u8[])- stamina scaling LUT
 *   D_800FF748 +0x796 + i*0x3E8 (u16) - per-player stamina (actor 1..22, +0x3AE)
 *   D_800FF748 +0xAC73 (u8) - cleared to 0
 *   D_800E7DE8        (u8)  - bit 0 suppresses 0x204 SFX in favor of 0x510
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SCRATCHPAD ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010A5A8[];
extern unsigned char D_8010865D;
extern unsigned char D_800EF78A[];
extern unsigned char D_8010C328[];
extern unsigned char D_800C6AD8[];
extern unsigned char D_800E7DE8;

extern void func_8004F25C(void);
extern void func_800AB620(int sfx_id);
extern void func_8001720C(void);

void func_8001EE84(int unused_a0, unsigned char *actor)
{
    unsigned char *scratch = SCRATCHPAD;
    unsigned char period;
    unsigned int half_len;
    unsigned int frames;
    unsigned int team_mod;
    unsigned char *p;
    int i;

    (void)unused_a0;

    B(scratch, 0x2A8) = (D_8010865D != 0) ? 0x16 : 0x0B;

    period = B(scratch, 0x2DF);
    W(scratch, 0x198) = 1;
    B(scratch, 0x2C2) = 0;
    W(scratch, 0x19C) = 2;
    B(scratch, 0x1E8) = 0;

    half_len = (unsigned int)B(D_8010A5A8, 0) + 1U;
    if (period < 2) {
        /* 9000 frames = 5 min * 60 s * 30 fps per half-length step */
        frames = half_len * 9000U;
    } else {
        /* 3000 frames = 100 s * 30 fps per half-length step (extra time) */
        frames = half_len * 3000U;
    }
    W(scratch, 0x2C4) = frames;
    W(scratch, 0x2C8) = 0x78;

    team_mod = B(D_800EF78A,
                 (unsigned int)B(actor, 0x392) * 4U + (unsigned int)B(actor, 0x398) * 88U) - 2U;

    p = D_800FF748 + 0x796;
    for (i = 0; i < 22; i++, p += 0x3E8) {
        unsigned char per = B(scratch, 0x2DF);
        if (per == 0 || per == 1) {
            unsigned int slot = B(p, -0x1C); /* actor+0x392 */
            unsigned int side = B(p, -0x16); /* actor+0x398 */
            unsigned int stat_off = slot * 12U + side * 264U;
            unsigned int stat = (W(D_8010C328, stat_off) >> 8) & 0xFU;
            unsigned int base = (unsigned int)B(D_800C6AD8, stat) + stat + team_mod;
            unsigned int stamina = base * 1000U + ((per == 0) ? 0x2328U : 0x1194U);

            H(p, 0) = (unsigned short)(stamina * ((unsigned int)B(D_8010A5A8, 0) + 1U));
        }
    }

    B(D_800FF748, 0xAC73) = 0;
    func_8004F25C();

    B(scratch, 0x1EC) = 0;
    if ((unsigned int)(B(scratch, 0x2E1) - 4U) >= 2U) {
        if (B(scratch, 0x2CE) == 0) {
            int sfx = 0x510;
            if (B(scratch, 0x298) == 3 && (D_800E7DE8 & 1) == 0) {
                sfx = 0x204;
            }
            func_800AB620(sfx);
        }
    }

    func_8001720C();
}
