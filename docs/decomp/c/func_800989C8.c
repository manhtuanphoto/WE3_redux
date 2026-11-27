/*
 * func_800989C8
 *
 * Purpose:
 *   Updates goal-net vertex deformation coordinates (D_800E6D90 / D_800E6DE8 /
 *   D_800E6DF0 / D_800E6DF8) when the ball is inside or against the goal frame
 *   and near the active camera X focus (Scratchpad+0x1FC). Dispatches by net
 *   zone/mode in Scratchpad+0xDE (0 = side/back net, 1 = deep/outer net, 3 = roof).
 *
 * Arguments:
 *   ball     - pointer to ball/object struct (+0x02/+0x04/+0x06 s16 x/y/z)
 *   resetFlg - low byte non-zero calls func_80098428(ball) first
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Ball:
 *     +0x002/+0x004/+0x006 (s16) x/y/z position (R)
 *   Scratchpad (0x1F800000):
 *     +0x0DE (u8)  goal net zone/mode selector (0, 1, 3) (R)
 *     +0x1FC (s32) camera/view X focus (R)
 *     +0x2F4 (u8*) pointer to ball actor (+0x04 s16 y) (R)
 *   Globals:
 *     D_800CA754..D_800CA75E (s16[6]) default net vertex coordinates (R)
 *     D_800E6D90..D_800E6DFC (s16) goal net vertex buffers (W)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x2001 = 8193 raw (~32.77 m) max X distance from camera focus
 *   0x3320 = 13088 raw (52.352 m) goal line X; 0x32E0 = 13024 raw (52.096 m)
 *   0x3341 = 13121 raw (52.484 m); 0x35A6 = 13734 raw (54.936 m)
 *   0x3360..0x343C = 13152..13372 raw (52.61..53.49 m) roof X span
 *   -0x266 = -614 raw (2.456 m) crossbar height; -0x2CE = -718 raw (2.872 m); -0x27E = -638 raw (2.552 m)
 *   0x3B2 / 0x3B3 = 946 / 947 raw (~3.78 m) half goal width; 0x592 = 1426 raw (~5.70 m)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800CA754[];
extern unsigned char D_800CA756[];
extern unsigned char D_800CA758[];
extern unsigned char D_800CA75A[];
extern unsigned char D_800CA75C[];
extern unsigned char D_800CA75E[];
extern unsigned char D_800E6D90[];
extern unsigned char D_800E6D92[];
extern unsigned char D_800E6D94[];
extern unsigned char D_800E6DE8[];
extern unsigned char D_800E6DEA[];
extern unsigned char D_800E6DEC[];
extern unsigned char D_800E6DF0[];
extern unsigned char D_800E6DF2[];
extern unsigned char D_800E6DF4[];
extern unsigned char D_800E6DF8[];
extern unsigned char D_800E6DFA[];
extern unsigned char D_800E6DFC[];

extern void func_80098428(unsigned char*);

void func_800989C8(unsigned char* ball, unsigned int resetFlg) {
    int x;
    int distX;
    unsigned char mode;

    if ((resetFlg & 0xFFu) != 0u) {
        func_80098428(ball);
    }

    x = S(ball, 0x2);
    distX = SW(Scratchpad, 0x1FC) - x;
    if (distX < 0) {
        distX = -distX;
    }
    if (distX >= 0x2001) {
        return;
    }

    mode = B(Scratchpad, 0x0DE);
    switch (mode) {
    case 0: {
        int absX;
        int z;
        int absZ;
        if (S(ball, 0x4) < -0x266) {
            return;
        }
        absX = (x >= 0) ? x : -x;
        if ((unsigned int)(absX - 0x3320) >= 0x243u) {
            return;
        }
        z = S(ball, 0x6);
        absZ = (z >= 0) ? z : -z;
        if (absZ >= 0x3B2) {
            return;
        }
        if (x > 0) {
            if (z > 0) {
                S(D_800E6DF8, 0) = (signed short)(z - 0x20);
                S(D_800E6DFA, 0) = (signed short)(S(ball, 0x4) - 5);
                S(D_800E6DFC, 0) = (signed short)(0x3320 - ((S(ball, 0x2) >= 0) ? S(ball, 0x2) : -S(ball, 0x2)));
            } else {
                S(D_800E6DF0, 0) = (signed short)(z + 0x20);
                S(D_800E6DF2, 0) = (signed short)(S(ball, 0x4) - 5);
                S(D_800E6DF4, 0) = (signed short)(0x3320 - ((S(ball, 0x2) >= 0) ? S(ball, 0x2) : -S(ball, 0x2)));
            }
        } else {
            if (z > 0) {
                S(D_800E6DF0, 0) = (signed short)(-z + 0x20);
                S(D_800E6DF2, 0) = (signed short)(S(ball, 0x4) - 5);
                S(D_800E6DF4, 0) = (signed short)(0x3320 - ((S(ball, 0x2) >= 0) ? S(ball, 0x2) : -S(ball, 0x2)));
            } else {
                S(D_800E6DF8, 0) = (signed short)(-z - 0x20);
                S(D_800E6DFA, 0) = (signed short)(S(ball, 0x4) - 5);
                S(D_800E6DFC, 0) = (signed short)(0x3320 - ((S(ball, 0x2) >= 0) ? S(ball, 0x2) : -S(ball, 0x2)));
            }
        }
        break;
    }
    case 1: {
        int absX = (x >= 0) ? x : -x;
        int z;
        int absZ;
        if (absX < 0x3341) {
            return;
        }
        if (S(ball, 0x4) < -0x2CE) {
            return;
        }
        z = S(ball, 0x6);
        absZ = (z >= 0) ? z : -z;
        if (absZ < 0x3B3) {
            if (absX < 0x35A6) {
                unsigned char* scratchBall = (unsigned char*)W(Scratchpad, 0x2F4);
                if (S(scratchBall, 0x4) >= -0x266) {
                    return;
                }
                S(D_800E6DE8, 0) = (signed short)((x > 0) ? z : -z);
                S(D_800E6DEA, 0) = (signed short)(S(ball, 0x4) - 0x40);
                S(D_800E6DEC, 0) = (signed short)(0x3320 - ((S(ball, 0x2) >= 0) ? S(ball, 0x2) : -S(ball, 0x2)));
            } else {
                int bx = S(ball, 0x2);
                S(D_800E6D90, 0) = (signed short)((x > 0) ? z : -z);
                S(D_800E6D92, 0) = (signed short)(S(ball, 0x4) - 5);
                S(D_800E6D94, 0) = (signed short)(0x32E0 - ((bx >= 0) ? bx : -bx));
                H(D_800E6DF0, 0) = H(D_800CA754, 0);
                H(D_800E6DF2, 0) = H(D_800CA756, 0);
                H(D_800E6DF4, 0) = H(D_800CA758, 0);
                H(D_800E6DF8, 0) = H(D_800CA75A, 0);
                H(D_800E6DFA, 0) = H(D_800CA75C, 0);
                H(D_800E6DFC, 0) = H(D_800CA75E, 0);
            }
        } else if (absZ < 0x592) {
            int bx = S(ball, 0x2);
            int absBx = (bx >= 0) ? bx : -bx;
            if (x > 0) {
                if (z > 0) {
                    S(D_800E6DF8, 0) = (signed short)(z + 0x40);
                    S(D_800E6DFA, 0) = (signed short)(S(ball, 0x4) - 2);
                    S(D_800E6DFC, 0) = (signed short)(0x3320 - absBx);
                } else {
                    S(D_800E6DF0, 0) = (signed short)(z - 0x40);
                    S(D_800E6DF2, 0) = (signed short)(S(ball, 0x4) - 2);
                    S(D_800E6DF4, 0) = (signed short)(0x3320 - absBx);
                }
            } else {
                if (z > 0) {
                    S(D_800E6DF0, 0) = (signed short)(-z - 0x40);
                    S(D_800E6DF2, 0) = (signed short)(S(ball, 0x4) - 2);
                    S(D_800E6DF4, 0) = (signed short)(0x3320 - absBx);
                } else {
                    S(D_800E6DF8, 0) = (signed short)(-z + 0x40);
                    S(D_800E6DFA, 0) = (signed short)(S(ball, 0x4) - 2);
                    S(D_800E6DFC, 0) = (signed short)(0x3320 - absBx);
                }
            }
        }
        break;
    }
    case 3: {
        int z;
        int absZ;
        int absX;
        int bx;
        if (S(ball, 0x4) < -0x27E) {
            return;
        }
        z = S(ball, 0x6);
        absZ = (z >= 0) ? z : -z;
        if (absZ >= 0x3B3) {
            return;
        }
        absX = (x >= 0) ? x : -x;
        if (absX >= 0x343D || absX < 0x3360) {
            return;
        }
        S(D_800E6DE8, 0) = (signed short)((z > 0) ? z : -z);
        S(D_800E6DEA, 0) = (signed short)(S(ball, 0x4) + 0x20);
        bx = S(ball, 0x2);
        S(D_800E6DEC, 0) = (signed short)(0x3320 - ((bx >= 0) ? bx : -bx));
        break;
    }
    default:
        break;
    }
}
