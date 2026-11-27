/*
 * func_800512B8 - Update controlled-player cursor assignments and lock timers
 *
 * Purpose:
 *   Dispatches cursor/control assignment commands (0..6) for a player actor,
 *   updating Scratchpad+0x240/0x241 (controlled actor IDs for P1/P2),
 *   Scratchpad+0x242/0x243 (cursor mode flags), Scratchpad+0x246/0x247
 *   (cursor lock timers), and actor+0x39A (cooperative cursor slot) depending
 *   on whether Scratchpad+0x2E2 is in coop/shared cursor mode (2 or 3).
 *
 * Arguments:
 *   actor (unsigned char *) - player actor pointer
 *   cmd   (unsigned char)   - cursor command (0..6; >=7 ignored)
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Actor fields read & written:
 *   Scratch 0x240/0x241 (u8) - controlled player actor IDs (written)
 *   Scratch 0x242[2]    (u8) - per-side cursor mode (1 or 2, written)
 *   Scratch 0x246[2]    (u8) - per-side cursor lock timer (0 or 10, written)
 *   Scratch 0x2AA       (u8) - previous ball owner ID
 *   Scratch 0x2AD/0x2AE (u8) - default/set-piece cursor actor IDs
 *   Scratch 0x2E2       (u8) - cursor control mode (2 or 3 = coop/2P shared)
 *   Scratch 0x2F4       (ptr)- ball actor pointer (+0x02 x, +0x06 z)
 *   Scratch 0x31E[4]    (u8) - tracked candidate actor IDs per attack direction
 *   Scratch 0x322[4]    (u8) - secondary candidate actor IDs per attack direction
 *   Actor +0x00         (u8) - active flag
 *   Actor +0x38D        (u8) - actor ID (1 and 12 = goalkeepers skipped in case 1)
 *   Actor +0x398        (u8) - team side (0/1)
 *   Actor +0x399        (u8) - attack direction (0/1)
 *   Actor +0x39A        (u8) - player cursor slot (0, 1, or 2; read & written)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   10 (0x0A) frames = 0.333 s cursor-switch lock timer written to 0x246/0x247.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];

extern unsigned char func_80049D58(unsigned char *actor);
extern void func_800676C4(unsigned int dir, int bx, int bz, int arg3);
extern void func_80066BD8(int arg0);
extern unsigned char func_8005CE30(unsigned int actor_id);

void func_800512B8(unsigned char *actor, unsigned int cmd)
{
    unsigned int op = cmd & 0xFF;

    if (op >= 7) {
        return;
    }

    switch (op) {
    case 0:
        if ((unsigned char)(B(Scratchpad, 0x2E2) - 2) >= 2) {
            unsigned char side = B(actor, 0x398);
            B(Scratchpad, 0x242 + side) = 1;
            B(Scratchpad, 0x246 + side) = 0;
        }
        break;

    case 1: {
        unsigned char id = B(actor, 0x38D);
        if (id == 1 || id == 12) {
            return;
        }
        if ((unsigned char)(B(Scratchpad, 0x2E2) - 2) < 2) {
            if (B(actor, 0x39A) == 1) {
                return;
            }
            if (B(Scratchpad, 0x240) == id || B(Scratchpad, 0x241) == id) {
                return;
            }
            if (B(Scratchpad, 0x240) == B(Scratchpad, 0x2AA)) {
                B(Scratchpad, 0x241) = id;
            } else {
                B(Scratchpad, 0x240) = id;
            }
            B(Scratchpad, 0x247) = 10;
            B(Scratchpad, 0x246) = 10;
        } else {
            unsigned char side = B(actor, 0x398);
            if (B(Scratchpad, 0x240 + side) == id) {
                return;
            }
            B(Scratchpad, 0x240 + side) = id;
            B(Scratchpad, 0x246 + B(actor, 0x398)) = 10;
        }
        break;
    }

    case 2:
        if ((unsigned char)(B(Scratchpad, 0x2E2) - 2) >= 2) {
            unsigned char side = B(actor, 0x398);
            B(Scratchpad, 0x242 + side) = 2;
            B(Scratchpad, 0x246 + side) = 0;
        }
        break;

    case 3:
        if ((unsigned char)(B(Scratchpad, 0x2E2) - 2) < 2) {
            if (B(actor, 0x39A) == 0) {
                B(Scratchpad, 0x240) = B(Scratchpad, 0x2AD);
                B(Scratchpad, 0x241) = B(Scratchpad, 0x2AE);
            } else {
                unsigned int opp_dir = B(actor, 0x399) ^ 1;
                B(Scratchpad, 0x240) = B(Scratchpad, 0x31E + opp_dir * 2);
                B(Scratchpad, 0x241) = B(Scratchpad, 0x31F + opp_dir * 2);
            }
        } else {
            unsigned char side = B(actor, 0x398);
            unsigned int opp_dir = B(actor, 0x399) ^ 1;
            B(Scratchpad, 0x240 + side) = B(actor, 0x38D);
            B(Scratchpad, 0x240 + (side ^ 1)) = B(Scratchpad, 0x31E + opp_dir * 2);
        }
        break;

    case 4:
        if ((unsigned char)(B(Scratchpad, 0x2E2) - 2) < 2) {
            if (B(actor, 0x39A) == 1) {
                return;
            }
            B(Scratchpad, 0x241) = B(actor, 0x38D);
            if (B(Scratchpad, 0x2E2) == 2) {
                B(actor, 0x39A) = 0;
            } else {
                B(actor, 0x39A) = 2;
            }
        } else {
            B(Scratchpad, 0x240 + (B(actor, 0x398) ^ 1)) = B(actor, 0x38D);
        }
        break;

    case 5:
        func_80066BD8(2);
        func_80066BD8(0xD);
        if ((unsigned char)(B(Scratchpad, 0x2E2) - 2) < 2) {
            if (B(actor, 0x39A) != 1) {
                B(Scratchpad, 0x240) = B(actor, 0x38D);
                B(actor, 0x39A) = 0;
                B(Scratchpad, 0x241) = B(Scratchpad, 0x31E + B(actor, 0x399) * 2);
            } else {
                unsigned int res = func_8005CE30(B(actor, 0x38D));
                B(Scratchpad, 0x241) = (unsigned char)res;
                if (B(Scratchpad, 0x2E2) == 3) {
                    B(D_800FF748 + (res % 24) * 1000, 0x39A) = 2;
                }
                B(Scratchpad, 0x240) = B(Scratchpad, 0x31E + (B(actor, 0x399) ^ 1) * 2);
            }
        } else {
            unsigned char side = B(actor, 0x398);
            unsigned char res;
            B(Scratchpad, 0x240 + side) = B(actor, 0x38D);
            res = func_8005CE30(B(actor, 0x38D));
            B(Scratchpad, 0x240 + (side ^ 1)) = res;
        }
        break;

    case 6:
        if ((unsigned char)(B(Scratchpad, 0x2E2) - 2) < 2) {
            if (B(actor, 0x39A) != 1) {
                unsigned int ret;
                B(Scratchpad, 0x240) = B(actor, 0x38D);
                B(actor, 0x39A) = 0;
                ret = func_80049D58(actor);
                B(Scratchpad, 0x241) = B(D_800FF748 + (ret % 24) * 1000, 0x38D);
            } else {
                unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
                unsigned int opp_dir = B(actor, 0x399) ^ 1;
                func_800676C4(opp_dir, S(ball, 2), S(ball, 6), 0);
                opp_dir = B(actor, 0x399) ^ 1;
                B(Scratchpad, 0x240) = B(Scratchpad, 0x322 + opp_dir * 2);
                B(Scratchpad, 0x241) = B(Scratchpad, 0x323 + opp_dir * 2);
            }
        } else {
            unsigned char *p;
            unsigned char *end;
            B(Scratchpad, 0x240 + B(actor, 0x398)) = B(actor, 0x38D);
            p = Scratchpad + 0x31E + (B(actor, 0x399) ^ 1) * 2;
            end = p + 2;
            while (p < end) {
                unsigned char *cand = D_800FF748 + ((unsigned int)B(p, 0) % 24) * 1000;
                if (B(cand, 0x00) != 0) {
                    B(Scratchpad, 0x240 + (B(actor, 0x398) ^ 1)) = B(cand, 0x38D);
                    break;
                }
                p++;
            }
        }
        break;
    }
}
