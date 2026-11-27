/*
 * func_800AB294
 *
 * Purpose:
 *   Audio/SPU subsystem initialization: resets sound state globals and channel
 *   tables, initializes SPU/CD sound drivers, streams VAB header and VAG body
 *   data from CD into RAM (0x801E0000 / 0x801E3000), patches VAB parameters,
 *   transfers waveforms to SPU RAM, and starts background/SFX audio playback.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   void
 *
 * Global fields read/written:
 *   D_800E6B88, D_800E6B76, D_800E6B34, D_800E6BD8, D_800E6AEC, D_800E6B4C,
 *   D_800E6B16, D_800E6AEA, D_800E6BD9, D_800E6B00, D_800E6B14, D_800E6B74,
 *   D_800E6BB8, D_800E6B90, D_800E6B15, D_800E6B5C, D_800E6ABC, D_800E6B44,
 *   D_800E6ADC, D_800E6AD6, D_800E6B6C (audio state/volume/timer globals)
 *   D_800EF898..D_800EF8A6 (8 u16 entries initialized to 0x270F = 9999)
 *   D_8010CD60..D_8010CD7E (16 u16 entries cleared to 0)
 *   D_800CE808..D_800CE826 (VAB stream descriptor table, stride 0x30)
 *   D_800CEA25[], D_800CE750, D_80014B38
 *   0x801E0000..0x801E3000+ (RAM staging buffers for VAB header and VAG chunks)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char D_800E6B88;
extern unsigned short D_800E6B76;
extern unsigned short D_800E6B34;
extern unsigned char D_800E6BD8;
extern unsigned short D_800E6AEC;
extern unsigned short D_800E6B4C;
extern unsigned short D_800E6B16;
extern unsigned char D_800E6AEA;
extern unsigned char D_800E6BD9;
extern unsigned char D_800E6B00;
extern unsigned char D_800E6B14;
extern unsigned char D_800E6B74;
extern unsigned int D_800E6BB8;
extern unsigned char D_800E6B90;
extern unsigned char D_800E6B15;
extern unsigned int D_800E6B5C;
extern signed short D_800E6ABC;
extern unsigned short D_800E6B44;
extern unsigned short D_800E6ADC;
extern unsigned short D_800E6AD6;
extern unsigned short D_800E6B6C;
extern unsigned char D_800EF8A6[];
extern unsigned char D_8010CD7E[];
extern unsigned char D_800CE808[];
extern unsigned char D_800CE80C[];
extern unsigned char D_800CE81C[];
extern unsigned char D_800CE820[];
extern unsigned char D_800CE824[];
extern unsigned char D_800CE826[];
extern unsigned char D_800CEA25[];
extern unsigned short D_800CE750;
extern unsigned char D_80014B38[];

extern void func_800BC718(int left, int right);
extern void func_800C0B98(int arg0);
extern void func_800A96C4(void);
extern void func_800BDD88(int arg0);
extern void func_800BDE38(void);
extern int func_800C39E8(int arg0);
extern void func_800BDCF8(int arg0, int arg1);
extern void func_800BC658(int arg0, int arg1, int arg2);
extern int func_800A9128(unsigned int sector, int count, unsigned char *dst);
extern int func_800C0BF8(unsigned char *hdr, int vabId, unsigned int arg2);
extern void func_800AD698(unsigned char *msg);
extern int func_800C1028(unsigned char *buf, int size, int vabId);
extern void func_800C1188(int arg0);
extern void func_800BCD68(int arg0);
extern void func_800BC9B8(void);
extern void func_800A95E8(void);
extern void func_800AB03C(void);
extern void func_800A9714(int arg0);
extern void func_800A98E0(int arg0);

void func_800AB294(void)
{
    int i;
    int s1;
    int s5;
    int off;

    D_800E6B88 = 1;
    D_800E6B76 = 0;
    D_800E6B34 = 0;
    D_800E6BD8 = 0;
    D_800E6AEC = 0;
    D_800E6B4C = 0;
    D_800E6B16 = 0;
    D_800E6AEA = 0;
    D_800E6BD9 = 0;
    D_800E6B00 = 0;
    D_800E6B14 = 0;
    D_800E6B74 = 1;
    D_800E6BB8 = 0;

    for (i = 0; i < 8; i++) {
        S(D_800EF8A6, -i * 2) = 0x270F;
    }
    for (i = 0; i < 16; i++) {
        S(D_8010CD7E, -i * 2) = 0;
    }

    D_800E6B90 = 0;
    D_800E6B15 = 0;
    D_800E6B5C = 0x801E1800U;

    func_800BC718(0, 0);
    func_800C0B98(0x16);
    func_800A96C4();
    func_800BDD88(2);
    func_800BDE38();
    s1 = func_800C39E8(2);
    func_800BDCF8(0x28, 0x28);
    func_800BC658(0, 0, 1);

    s5 = 0;
    off = 0;
    do {
        if (D_800CE826[off] == 1) {
            int chunk = 0x10000;
            unsigned int sector = W(D_800CE80C, off);
            unsigned char *vabBuf = (unsigned char *)0x801E0000;
            int remaining;

            do {
                if (D_800CE824[off] == 0) {
                    s1 = func_800A9128(W(D_800CE808, off), (int)sector, vabBuf);
                }
            } while (s1 != 0);

            /* UNCLEAR: if D_800CE824[off] != 0, original asm leaves $s7 unchanged */
            B(vabBuf, 0xA02) = 0x60;
            B(vabBuf, 0x902) = 0x6F;
            B(vabBuf, 0x904) = 0x32;
            B(vabBuf, 0x913) = 0x4C;
            B(vabBuf, 0x916) = 0x05;

            s1 = (signed short)func_800C0BF8(vabBuf, D_800CE824[off], 0x41010U);

            B(vabBuf, 0x9E2) = 0x7F;
            B(vabBuf, 0x9E4) = 0x50;
            B(vabBuf, 0x9F6) = 0x06;
            B(vabBuf, 0xAB6) = 0x0B;
            B(vabBuf, 0xAF6) = 0x0B;

            if (s1 == -1) {
                func_800AD698(D_80014B38);
            }

            remaining = SW(D_800CE820, off) << 11;
            sector = W(D_800CE81C, off);
            if (remaining < chunk) {
                chunk = remaining;
            }

            while (remaining > 0) {
                unsigned char *vagBuf = (unsigned char *)0x801E3000;
                int ret;

                do {
                    ret = func_800A9128(sector, chunk / 2048, vagBuf);
                } while (ret != 0);

                sector += 0x20;
                remaining -= chunk;
                s1 = (signed short)func_800C1028(vagBuf, chunk, D_800CE824[off]);
                func_800C1188(1);
                if (remaining < chunk) {
                    chunk = remaining;
                }
            }
        }
        s5++;
        off += 0x30;
    } while (s5 <= 0);

    func_800BCD68(0x1000);
    D_800E6B44 = D_800CEA25[D_800E6ABC];
    func_800BC718(0x7F, 0x7F);
    func_800BC9B8();
    D_800E6B88 = 2;
    D_800E6B4C = 0;
    func_800A95E8();
    func_800AB03C();
    func_800A9714(0xC);
    func_800A98E0(0xC);
    D_800E6ADC = 0;
    D_800E6AD6 = 0;
    D_800E6B6C = D_800CE750;
}
