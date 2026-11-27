/*
 * func_800A71FC
 *
 * Purpose:
 *   Repositions all 22 players across both teams (e.g., for kickoff / half /
 *   set-piece reset) based on which half of the pitch the ball is in, then
 *   finalizes formation/actor updates.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800FF748 (actor table base, 1000-byte / 0x3E8 stride; team 0 at +0x3E8, team 1 at +0x2EE0)
 *
 * Actor fields read:
 *   +0x002 (s16) X position
 *   +0x006 (s16) Z position
 *   +0x38D (u8)  actor ID (1 = team 0 GK, 12/0xC = team 1 GK)
 *   +0x391 (u8)  formation / role slot
 *   +0x399 (u8)  team side (0 or 1)
 *
 * Actor fields written:
 *   +0x002 (s16) clamped X position (+/-0x2382 = +/-9090 if near goal line)
 *   +0x396 (u8)  state (0x80 for GKs, 0 for outfield players)
 *   +0x397 (u8)  substate (0)
 *
 * Scratchpad (0x1F800000) fields read/written:
 *   +0x2AD (u8)  assigned taker/player ID (if 0xFF, initialized via func_8007BA04)
 *   +0x2F4 (u32) ball pointer
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];

extern unsigned char func_8007BA04(unsigned char *team_base);
extern void func_8009F554(unsigned char *actor, signed short x, signed short z);
extern void func_8005A85C(unsigned char *actor, signed short *out_x, signed short *out_z);
extern signed short func_8005A76C(signed short val, signed short target, int weight);
extern void func_800A3730(void);
extern void func_8009F574(void);

void func_800A71FC(void)
{
    unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
    unsigned int ball_left_half = (S(ball, 2) < 1) ? 1U : 0U;
    unsigned int team;
    unsigned int i;

    if (B(Scratchpad, 0x2AD) == 0xFF) {
        unsigned char *team_base = (ball_left_half != 0)
            ? (D_800FF748 + 0x2EE0)
            : (D_800FF748 + 0x3E8);
        B(Scratchpad, 0x2AD) = func_8007BA04(team_base);
    }

    for (team = 0; team < 2; team++) {
        unsigned char *actor = (team == 0)
            ? (D_800FF748 + 0x3E8)
            : (D_800FF748 + 0x2EE0);

        for (i = 0; i < 11; i++, actor += 0x3E8) {
            unsigned char actor_id = B(actor, 0x38D);
            signed short tx;
            signed short tz;
            signed short abs_z;
            signed short abs_x;

            if (actor_id == 1) {
                func_8009F554(actor, -0x3120, 0);
                B(actor, 0x396) = 0x80;
                B(actor, 0x397) = 0;
                continue;
            }
            if (actor_id == 0xC) {
                func_8009F554(actor, 0x3120, 0);
                B(actor, 0x396) = 0x80;
                B(actor, 0x397) = 0;
                continue;
            }

            func_8005A85C(actor, &tx, &tz);

            if ((ball_left_half & 0xFFU) == B(actor, 0x399)) {
                unsigned char slot = B(actor, 0x391);
                if (slot != 0 && slot < 0xE) {
                    unsigned char *b = (unsigned char *)W(Scratchpad, 0x2F4);
                    tx = func_8005A76C(tx, S(b, 2), 7);
                    b = (unsigned char *)W(Scratchpad, 0x2F4);
                    tz = func_8005A76C(tz, S(b, 6), 5);
                }
            } else {
                unsigned char slot = B(actor, 0x391);
                if (slot != 0 && slot < 9) {
                    tx = func_8005A76C(tx, 0, 2);
                } else {
                    tz = func_8005A76C(tz, 0, 5);
                }
            }

            func_8009F554(actor, tx, tz);

            abs_z = S(actor, 6);
            if (abs_z < 0) {
                abs_z = (signed short)-abs_z;
            }
            if (abs_z < 0x12F4) {
                abs_x = S(actor, 2);
                if (abs_x < 0) {
                    abs_x = (signed short)-abs_x;
                }
                if (abs_x >= 0x23E2) {
                    S(actor, 2) = (ball_left_half != 0) ? -0x2382 : 0x2382;
                }
            }

            B(actor, 0x396) = 0;
            B(actor, 0x397) = 0;
        }
    }

    func_800A3730();
    func_8009F574();
}
