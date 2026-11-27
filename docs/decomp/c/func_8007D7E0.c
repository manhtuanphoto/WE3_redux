/*
 * func_8007D7E0 - Goal celebration / scoreboard banner sequence tick
 *
 * Purpose:
 *   Per-frame update during the goal-scored presentation sequence. Runs camera/
 *   scorer tracking (func_800477A0 on Scratchpad+0x2AD with mode 5), audio/
 *   match helpers, slides the two team banner X positions toward -80 and +80,
 *   decrements banner timers D_8010597A/D_80105952, spawns the score digit
 *   sprites at frame 40 (1.33 s), animates the goal-scorer sprite scale/bounce
 *   between frames 41 and 181+ (1.37..6.03 s), and exits to state 3 on button
 *   skip (func_800525A4) or after frame 211 (7.03 s).
 *
 * Arguments:
 *   None (void).
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Actor / Global fields read & written:
 *   Scratch 0x294      (u16) - sequence frame timer (incremented each call)
 *   Scratch 0x2AD      (u8)  - goal scorer actor ID
 *   Scratch 0x2D1[2]   (u8)  - team scores
 *   Scratch 0x2E0      (u8)  - home/left side index (0 or 1)
 *   Actor +0x399       (u8)  - scorer attack direction
 *   D_800FF748+0xAC28  (s16[2]) - team banner X offsets (clamped to -80 / +80)
 *   D_800FF748+0xABDB  (u8)  - XOR mask for scorer sprite slot
 *   D_800FF748+0x6248  (80-byte structs) - +0x1E/+0x46 s16 scale/bounce value
 *   D_8010597A / D_80105952 (s16/u16) - banner fade/scroll counters (-2/frame)
 *   D_800E6D58         (s16) - bounce velocity (-0x4000 init, +/-0x400 step)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   40  (0x28) frames = 1.333 s: spawn score digits (ASCII/glyph base 0x33 = '0')
 *   41  (0x29) frames = 1.367 s: start bounce animation
 *   181 (0xB5) frames = 6.033 s: settle bounce at 0x1000 (1.0 scale)
 *   211 (0xD3) frames = 7.033 s: auto-finish presentation
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern signed short D_8010597A;
extern unsigned short D_80105952;
extern signed short D_800E6D58;

extern void func_800477A0(unsigned char *actor, int mode);
extern void func_8002283C(void);
extern void func_8004A2D0(void);
extern void func_80051810(void);
extern void func_8001CEE0(int id, int glyph, int a2, int x, int y, int a5, int a6, int a7);
extern int func_800525A4(void);
extern void func_800AB620(int id);
extern void func_800171FC(int state);

void func_8007D7E0(void)
{
    unsigned int scorer_id = B(Scratchpad, 0x2AD);
    unsigned char team0;
    unsigned char team1;
    unsigned short timer;
    unsigned char *sprite;
    unsigned char *scorer;
    unsigned int idx;

    func_800477A0(D_800FF748 + (scorer_id % 24) * 1000, 5);
    func_8002283C();
    func_8004A2D0();
    func_80051810();

    team0 = B(Scratchpad, 0x2E0);
    S(D_800FF748, 0xAC28 + team0 * 2) += 4;
    if (S(D_800FF748, 0xAC28 + team0 * 2) >= -0x4F) {
        S(D_800FF748, 0xAC28 + team0 * 2) = -0x50;
    }

    team1 = B(Scratchpad, 0x2E0) ^ 1;
    S(D_800FF748, 0xAC28 + team1 * 2) -= 4;
    if (S(D_800FF748, 0xAC28 + team1 * 2) < 0x50) {
        S(D_800FF748, 0xAC28 + team1 * 2) = 0x50;
    }

    if (D_8010597A > 0) {
        D_8010597A -= 2;
        D_80105952 -= 2;
    }

    if (H(Scratchpad, 0x294) == 0x28) {
        unsigned int s0 = B(Scratchpad, 0x2D1 + B(Scratchpad, 0x2E0));
        unsigned int s1;

        if (s0 < 10) {
            func_8001CEE0(0x1D, s0 + 0x33, 0, -0x34, -0x48, 6, 0, 1);
        } else {
            func_8001CEE0(0x1D, (s0 / 10) + 0x33, 0, -0x34, -0x48, 6, 0, 1);
            s0 = B(Scratchpad, 0x2D1 + B(Scratchpad, 0x2E0));
            func_8001CEE0(0x1E, (s0 % 10) + 0x33, 0, -0x1C, -0x48, 6, 0, 1);
        }

        s1 = B(Scratchpad, 0x2D1 + (B(Scratchpad, 0x2E0) ^ 1));
        if (s1 < 10) {
            func_8001CEE0(0x1F, s1 + 0x33, 0, 0x30, -0x48, 6, 0, 1);
        } else {
            func_8001CEE0(0x1F, (s1 / 10) + 0x33, 0, 0x18, -0x48, 6, 0, 1);
            s1 = B(Scratchpad, 0x2D1 + (B(Scratchpad, 0x2E0) ^ 1));
            func_8001CEE0(0x20, (s1 % 10) + 0x33, 0, 0x30, -0x48, 6, 0, 1);
        }

        D_800E6D58 = -0x4000;
    }

    scorer = D_800FF748 + ((unsigned int)B(Scratchpad, 0x2AD) % 24) * 1000;
    idx = B(scorer, 0x399) ^ B(D_800FF748, 0xABDB);
    sprite = D_800FF748 + 0x6248 + idx * 80;

    timer = H(Scratchpad, 0x294);
    if (timer >= 0x29) {
        if (timer >= 0xB5 && S(sprite, 0x1E) == 0x1000) {
            S(sprite, 0x46) = 0x1000;
        } else {
            signed short val = (signed short)(H(sprite, 0x1E) + D_800E6D58);
            S(sprite, 0x1E) = val;
            if (val >= 0x1001) {
                S(sprite, 0x1E) = 0x1000;
                D_800E6D58 = -0x400;
            }
            if (S(sprite, 0x1E) < -0x1000) {
                S(sprite, 0x1E) = -0x1000;
                D_800E6D58 = 0x400;
            }
            S(sprite, 0x46) = S(sprite, 0x1E);
        }
    }

    H(Scratchpad, 0x294) = H(Scratchpad, 0x294) + 1;
    if (func_800525A4() != 0) {
        func_800AB620(0x11);
        func_800171FC(3);
    } else if (H(Scratchpad, 0x294) >= 0xD3) {
        func_800171FC(3);
    }
}
