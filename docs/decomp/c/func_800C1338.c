/*
 * func_800C1338
 *
 * Purpose:
 *   Initializes the PS1 SPU (Sound Processing Unit) hardware and driver state
 *   (equivalent to libspu `_SpuInit` / cold-or-hot init). Sets hardware config
 *   bits in *D_800D559C, resets SPU control/status registers at *D_800D558C,
 *   waits for SPU status bits (0x7FF) to clear (with 0xF00-iteration timeout),
 *   configures SPU SRAM allocation parameters (D_800D55B0..D_800D55BC), clears
 *   D_800F0EA8[0..9], and when `mode == 0` (cold init) zeroes all 24 SPU voice
 *   registers, clears reverb/noise/pitch-mod registers, and performs key-off /
 *   key-on/status resets via func_800C1E10.
 *
 * Arguments:
 *   mode (int) - 0 for full cold init (resets all 24 voices and SPU RAM block),
 *                non-zero for warm/partial init.
 *
 * Return value:
 *   int - always 0.
 *
 * Hardware / Global fields:
 *   D_800D559C (u32 *) - pointer to PS1 memory-delay/bus config register (ORed with 0xB0000)
 *   D_800D558C (u8 *)  - pointer to SPU MMIO base (0x1F801C00):
 *     +0x000..+0x17F: 24 voices (stride 0x10: volL, volR, pitch, addr, adsr1, adsr2)
 *     +0x180/+0x182: main volume L/R
 *     +0x184/+0x186: reverb output volume L/R
 *     +0x188/+0x18A: key on low/high
 *     +0x18C/+0x18E: key off low/high
 *     +0x190..+0x196: pitch mod / noise enable
 *     +0x198/+0x19A: reverb enable
 *     +0x1AA: SPUCNT (set to 0, then 0xC000)
 *     +0x1AC: SPU transfer mode (set to 4)
 *     +0x1AE: SPUSTAT (bits 0..10 polled for 0)
 *     +0x1B0..+0x1B6: CD / ext audio volume
 *   D_800D55B0..D_800D55BC - SPU allocator constants (2, 3, 8, 7)
 *   D_800F0EA8 (u16[10])   - cleared to 0
 */

#define H(p, o) (*(unsigned short *)((p) + (o)))

extern unsigned int *D_800D559C;
extern unsigned char *D_800D558C;
extern unsigned short D_800D55A4;
extern unsigned int D_800D55A8;
extern unsigned int D_800D55AC;
extern unsigned int D_800D55B0;
extern unsigned int D_800D55B4;
extern unsigned int D_800D55B8;
extern unsigned int D_800D55BC;
extern unsigned int D_800D55C0;
extern unsigned int D_800D55C4;
extern unsigned int D_800D55C8;
extern unsigned char D_800D55CC[];
extern unsigned short D_800F0EA8[];
extern char D_8001551C[];
extern char D_8001552C[];

extern void func_800C1E10(void);
extern void func_800AD698(const char *fmt, ...);
extern void func_800C15B8(unsigned char *buf, int count);

int func_800C1338(int mode)
{
    unsigned char *spu;
    unsigned int timeout;
    int i;

    *D_800D559C |= 0xB0000U;

    spu = D_800D558C;
    D_800D55A8 = 0;
    D_800D55AC = 0;
    D_800D55A4 = 0;
    H(spu, 0x180) = 0;
    H(spu, 0x182) = 0;
    H(spu, 0x1AA) = 0;

    func_800C1E10();

    spu = D_800D558C;
    H(spu, 0x180) = 0;
    H(spu, 0x182) = 0;

    if ((H(spu, 0x1AE) & 0x7FF) != 0) {
        timeout = 1;
        while (timeout < 0xF01U) {
            if ((H(D_800D558C, 0x1AE) & 0x7FF) == 0) {
                break;
            }
            timeout++;
        }
        if (timeout >= 0xF01U) {
            func_800AD698(D_8001551C, D_8001552C);
        }
    }

    D_800D55B0 = 2;
    D_800D55B4 = 3;
    D_800D55B8 = 8;
    D_800D55BC = 7;

    spu = D_800D558C;
    H(spu, 0x1AC) = 4;
    H(spu, 0x184) = 0;
    H(spu, 0x186) = 0;
    H(spu, 0x18C) = 0xFFFF;
    H(spu, 0x18E) = 0xFFFF;
    H(spu, 0x198) = 0;
    H(spu, 0x19A) = 0;

    for (i = 0; i < 10; i++) {
        D_800F0EA8[i] = 0;
    }

    if (mode == 0) {
        unsigned char *voice;

        D_800D55A4 = 0x200;
        spu = D_800D558C;
        H(spu, 0x190) = 0;
        H(spu, 0x192) = 0;
        H(spu, 0x194) = 0;
        H(spu, 0x196) = 0;
        H(spu, 0x1B0) = 0;
        H(spu, 0x1B2) = 0;
        H(spu, 0x1B4) = 0;
        H(spu, 0x1B6) = 0;

        func_800C15B8(D_800D55CC, 0x10);

        voice = D_800D558C;
        for (i = 0; i < 24; i++, voice += 0x10) {
            H(voice, 0x0) = 0;
            H(voice, 0x2) = 0;
            H(voice, 0x4) = 0x3FFF;
            H(voice, 0x6) = 0x200;
            H(voice, 0x8) = 0;
            H(voice, 0xA) = 0;
        }

        spu = D_800D558C;
        H(spu, 0x188) = 0xFFFF;
        H(spu, 0x18A) = 0xFF;
        func_800C1E10();
        func_800C1E10();
        func_800C1E10();
        func_800C1E10();

        spu = D_800D558C;
        H(spu, 0x18C) = 0xFFFF;
        H(spu, 0x18E) = 0xFF;
        func_800C1E10();
        func_800C1E10();
        func_800C1E10();
        func_800C1E10();
    }

    spu = D_800D558C;
    D_800D55C0 = 1;
    H(spu, 0x1AA) = 0xC000;
    D_800D55C4 = 0;
    D_800D55C8 = 0;

    return 0;
}
