/*
 * func_800AED64
 *
 * Purpose:
 *   PsyQ PutDispEnv equivalent. Applies a 20-byte DISPENV struct to PS1 GPU GP1
 *   control registers (display start, horizontal/vertical display range, and
 *   display mode) when fields differ from the cached DISPENV at D_800CEB30,
 *   then caches the new DISPENV.
 *
 * Arguments:
 *   dispEnv - pointer to 20-byte DISPENV struct:
 *     +0x00/+0x02 (s16) disp.x, disp.y (VRAM origin)
 *     +0x04/+0x06 (s16) disp.w, disp.h
 *     +0x08/+0x0A (s16) screen.x, screen.y
 *     +0x0C/+0x0E (s16) screen.w, screen.h
 *     +0x10 (u8) isinter, +0x11 (u8) isrgb24, +0x12 (u8) pad0 (set to video mode)
 *
 * Return value:
 *   Returns dispEnv.
 *
 * Reads / Writes:
 *   dispEnv:
 *     +0x00..+0x13 (R), +0x12 (W: video mode from func_800BB22C)
 *   Globals:
 *     D_800CEAC0 (void(*)(u8*, u8*)) debug logger callback (R)
 *     D_800CEAC6 (u8) GPU debug level; +0x72 = D_800CEB38 (cached screen rect) (R)
 *     D_800CEAC7 (u8) reverse/flag bit 0x80 for GP1(08h) (R)
 *     D_800CEABC (u8*) pointer to GPU driver table (+0x10 = GP1 write fn) (R)
 *     D_800CEB30 (20 bytes) cached DISPENV (+0x10 = D_800CEB40) (R/W)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   GP1 commands: 0x05000000 (display start), 0x06000000 (H range), 0x07000000 (V range), 0x08000000 (mode)
 *   Horizontal range clamp: [0x1F4 (500), 0xCDA (3290)], default width 0xA00 (2560), min width 0x50 (80)
 *   Vertical range clamp: NTSC [0x10 (16), 0x100/0x102], PAL [0x10, 0x136/0x138], default height 0xF0 (240)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_80014FF0[];
extern unsigned char D_800CEABC[];
extern unsigned char D_800CEAC0[];
extern unsigned char D_800CEAC6[];
extern unsigned char D_800CEAC7[];
extern unsigned char D_800CEB30[];
extern unsigned char D_800CEB40[];

extern unsigned char func_800BB22C(void);
extern void func_800B1118(unsigned char*, unsigned char*, int);

unsigned char* func_800AED64(unsigned char* dispEnv) {
    unsigned char* gpuCtx = D_800CEAC6;
    unsigned int gp1Mode = 0x08000000u;
    void (*gp1Write)(unsigned int);

    if (B(gpuCtx, 0) >= 2) {
        ((void (*)(unsigned char*, unsigned char*))W(D_800CEAC0, 0))(D_80014FF0, dispEnv);
    }

    gp1Write = (void (*)(unsigned int))W((unsigned char*)W(D_800CEABC, 0), 0x10);
    gp1Write(0x05000000u | (((unsigned int)H(dispEnv, 0x2) & 0x3FFu) << 10) | ((unsigned int)H(dispEnv, 0x0) & 0x3FFu));

    if (S(gpuCtx, 0x72) != S(dispEnv, 0x8) ||
        S(gpuCtx, 0x74) != S(dispEnv, 0xA) ||
        S(gpuCtx, 0x76) != S(dispEnv, 0xC) ||
        S(gpuCtx, 0x78) != S(dispEnv, 0xE)) {
        unsigned char isPal = func_800BB22C();
        int scrX;
        int scrY;
        int scrW;
        int scrH;
        int x1;
        int x2;
        int y1;
        int y2;

        B(dispEnv, 0x12) = isPal;
        scrX = S(dispEnv, 0x8);
        x1 = scrX * 10 + 0x260;
        scrY = S(dispEnv, 0xA);
        y1 = ((isPal & 0xFFu) != 0u) ? (scrY + 0x13) : (scrY + 0x10);

        scrW = S(dispEnv, 0xC);
        x2 = (scrW != 0) ? (x1 + scrW * 10) : (x1 + 0xA00);
        scrH = S(dispEnv, 0xE);
        y2 = (scrH != 0) ? (y1 + scrH) : (y1 + 0xF0);

        if (x1 < 0x1F4) {
            x1 = 0x1F4;
        } else if (x1 >= 0xCDB) {
            x1 = 0xCDA;
        }

        if (x2 < x1 + 0x50) {
            x2 = x1 + 0x50;
        } else if (x2 >= 0xCDB) {
            x2 = 0xCDA;
        }

        if (y1 < 0x10) {
            y1 = 0x10;
        } else if (B(dispEnv, 0x12) != 0) {
            if (y1 >= 0x137) {
                y1 = 0x136;
            }
        } else {
            if (y1 >= 0x101) {
                y1 = 0x100;
            }
        }

        if (y2 < y1 + 2) {
            y2 = y1 + 2;
        } else if (B(dispEnv, 0x12) != 0) {
            if (y2 >= 0x139) {
                y2 = 0x138;
            }
        } else {
            if (y2 >= 0x103) {
                y2 = 0x102;
            }
        }

        gp1Write = (void (*)(unsigned int))W((unsigned char*)W(D_800CEABC, 0), 0x10);
        gp1Write(0x06000000u | (((unsigned int)x2 & 0xFFFu) << 12) | ((unsigned int)x1 & 0xFFFu));

        gp1Write = (void (*)(unsigned int))W((unsigned char*)W(D_800CEABC, 0), 0x10);
        gp1Write(0x07000000u | (((unsigned int)y2 & 0x3FFu) << 10) | ((unsigned int)y1 & 0x3FFu));
    }

    if (W(D_800CEB40, 0) != W(dispEnv, 0x10) ||
        S(D_800CEB40 - 0x10, 0x0) != S(dispEnv, 0x0) ||
        S(D_800CEB40 - 0x10, 0x2) != S(dispEnv, 0x2) ||
        S(D_800CEB40 - 0x10, 0x4) != S(dispEnv, 0x4) ||
        S(D_800CEB40 - 0x10, 0x6) != S(dispEnv, 0x6)) {
        unsigned char isPal = func_800BB22C();
        int dispW;
        int dispH;
        int highV;

        B(dispEnv, 0x12) = isPal;
        if ((isPal & 0xFFu) == 1u) {
            gp1Mode |= 0x8u;
        }
        if (B(dispEnv, 0x11) != 0) {
            gp1Mode |= 0x10u;
        }
        if (B(dispEnv, 0x10) != 0) {
            gp1Mode |= 0x20u;
        }
        if (B(D_800CEAC7, 0) != 0) {
            gp1Mode |= 0x80u;
        }

        dispW = S(dispEnv, 0x4);
        if (dispW < 0x119) {
            /* 256px mode */
        } else if (dispW < 0x161) {
            gp1Mode |= 0x1u;
        } else if (dispW < 0x191) {
            gp1Mode |= 0x40u;
        } else if (dispW < 0x231) {
            gp1Mode |= 0x2u;
        } else {
            gp1Mode |= 0x3u;
        }

        dispH = S(dispEnv, 0x6);
        if (B(dispEnv, 0x12) != 0) {
            highV = (dispH < 0x121);
        } else {
            highV = (dispH < 0x101);
        }
        if (!highV) {
            gp1Mode |= 0x24u;
        }

        gp1Write = (void (*)(unsigned int))W((unsigned char*)W(D_800CEABC, 0), 0x10);
        gp1Write(gp1Mode);
    }

    func_800B1118(D_800CEB30, dispEnv, 0x14);
    return dispEnv;
}
