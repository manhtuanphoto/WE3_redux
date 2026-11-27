/*
 * func_800AC27C
 *
 * Purpose:
 *   Per-frame CD-ROM / XA audio playback and volume-fade state machine. Updates
 *   frame/second timers and playback elapsed time, polls CD status/result via
 *   func_800B6FCC/func_800B6FEC/func_800B7034, drives volume fade-in/fade-out
 *   via func_800BCC58, and dispatches track start/stop/transition helpers
 *   according to state D_800E6B4C.
 *
 * Arguments:
 *   None (void).
 *
 * Return value:
 *   None (void).
 *
 * Actor fields read/written:
 *   None.
 *
 * Scratchpad fields read/written:
 *   None.
 *
 * Globals read/written:
 *   D_800E6B0C (u32/s32 frame counter within second, wraps at 0x3C = 60)
 *   D_800E6B9C (u32 seconds counter)
 *   D_8010CDDC (u32/u16 frame delta)
 *   D_800E6A8C (u32 passed to func_800C39B8)
 *   D_800E6AB0 (s32 playback active flag: 0/1/2)
 *   D_800E6AAC (s32 elapsed playback counter, +5 * D_8010CDDC per tick)
 *   D_800E6AB4 (s32 target track duration)
 *   D_800E6AEA (u8 pause/disable flag; non-zero skips state machine)
 *   D_800E6B4C (s16/u16 state machine state 0..93)
 *   D_800E6B76 (u16 track/XA id; 0x200..0x2FF vs >= 0x300)
 *   D_800E6AD6 (s16/u16 primary/left fade volume, max 0x75 = 117)
 *   D_800E6ADC (s16/u16 secondary/right fade volume)
 *   D_800E6B6C (s16 master volume scale, product >> 7)
 *   D_800E6B16 (s16 fade-out step)
 *   D_800E6BB2 (u16 wait timer, threshold 0xB5 = 181)
 *   D_800E6BA4 (s16 retry/startup counter, max 0x65 = 101)
 *   D_800E6B00 (u8 stall/retry counter, threshold 0x2E = 46)
 *   D_800E6AF0 / D_800E6AF1 (u8 CD command result bytes)
 *   D_800E6AF8 / D_800E6AF9 (u8 CD status bytes)
 *   D_800E6B28, D_800E6B44, D_800E6B58, D_800E6BA0, D_800E6ABA, D_800E6BD8
 *   D_800CE9D0, D_800CE9D4 (track descriptor tables)
 *
 * Magic numbers / unit conversions:
 *   0x3C  = 60 ticks/s (NTSC VSync rate for D_800E6B0C)
 *   0x75  = 117 (max fade volume before scaling by D_800E6B6C / 128)
 *   0x520 = sound effect id passed to func_800AB620
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern int D_800E6B0C;
extern int D_800E6B9C;
extern unsigned int D_8010CDDC;
extern unsigned int D_800E6A8C;
extern int D_800E6AB0;
extern int D_800E6AAC;
extern int D_800E6AB4;
extern unsigned char D_800E6AEA;
extern signed short D_800E6B4C;
extern unsigned short D_800E6B76;
extern unsigned short D_800E6AD6;
extern signed short D_800E6ADC;
extern signed short D_800E6B6C;
extern signed short D_800E6B16;
extern unsigned short D_800E6BB2;
extern signed short D_800E6BA4;
extern unsigned char D_800E6B00;
extern unsigned char D_800E6AF0[];
extern unsigned char D_800E6AF1;
extern unsigned char D_800E6AF8[];
extern unsigned char D_800E6AF9;
extern signed short D_800E6B28;
extern signed short D_800E6B44;
extern signed short D_800E6B58;
extern unsigned char D_800E6BA0;
extern unsigned char D_800E6ABA;
extern unsigned char D_800E6BD8;
extern unsigned char D_800CE9D0[];
extern unsigned char D_800CE9D4[];

extern void func_800C39B8(int arg0, unsigned int arg1);
extern void func_800AA7C4(void);
extern void func_800AA9E8(void);
extern int func_800B6FCC(int mode, void *out);
extern int func_800B6FEC(int mode, void *out);
extern int func_800B7034(int mode, void *arg1, void *arg2);
extern void func_800AC160(void);
extern void func_800AC084(void);
extern void func_800AC074(void);
extern void func_800A95E8(void);
extern void func_800A95C0(void);
extern void func_800A9568(void);
extern void func_800AB620(int sfx_id);
extern void func_800ABC34(void);
extern void func_800BCC58(int arg0, int vol_l, int vol_r);
extern void func_800ACD08(void);
extern void func_800ACC80(void);
extern void func_800AAAD8(void);

void func_800AC27C(void)
{
    unsigned char buf[16];
    int vol_l;
    int vol_r;
    int state;

    if (D_800E6B0C >= 0x3C) {
        D_800E6B0C = 0;
        D_800E6B9C++;
    } else {
        D_800E6B0C += (int)D_8010CDDC;
    }

    func_800C39B8(1, D_800E6A8C);
    func_800AA7C4();
    func_800AA9E8();

    if (D_800E6AB0 == 0) {
        D_800E6AAC += (int)(D_8010CDDC * 5);
    }

    if (D_800E6AEA != 0) {
        return;
    }

    state = D_800E6B4C;
    if ((unsigned int)state >= 0x5EU) {
        return;
    }

    switch (state) {
    case 0:
        D_800E6AB0 = 2;
        return;

    case 4:
        if (D_800E6BA4 < 0x65) {
            D_800E6BA4++;
        }
        if (D_800E6BA4 == 0x3C) {
            func_800ABC34();
            D_800E6B28 = 1;
        }
        if (D_800E6BA4 >= 0x5A) {
            D_800E6B28 = 1;
            if (func_800B6FCC(1, D_800E6AF0) == 2) {
                if ((D_800E6B0C & 3) == 0) {
                    func_800ABC34();
                }
                D_800E6B44 = (signed short)(D_800E6B76 - 0x1FE);
            }
            if (func_800B6FEC(1, 0) == 4) {
                if (B(D_800CE9D4, 3 + (D_800E6B76 - 0x200) * 6) == 0) {
                    func_800AC084();
                } else {
                    D_800E6B4C = 0;
                    func_800A95E8();
                    D_800E6B76 = 0;
                }
            }
        }
        if (D_800E6BD8 == 1) {
            D_800E6B4C = 0x45;
            return;
        }
        /* fall through */

    case 3:
        if (D_800E6B16 != 0) {
            int next_vol = (signed short)(D_800E6AD6 - D_800E6B16);
            D_800E6AD6 = (unsigned short)next_vol;
            if (next_vol <= 0) {
                D_800E6AD6 = 0;
                D_800E6B16 = 0;
                func_800A95E8();
                D_800E6B4C = 0;
                D_800E6B76 = 0;
            }
            D_800E6ADC = (signed short)D_800E6AD6;
            vol_l = (signed short)(((signed short)D_800E6AD6 * D_800E6B6C) / 128);
            func_800BCC58(0, vol_l, vol_l);
        }
        /* fall through */

    case 1:
        vol_l = (signed short)(((signed short)D_800E6AD6 * D_800E6B6C) / 128);
        vol_r = (signed short)((D_800E6ADC * D_800E6B6C) / 128);
        func_800BCC58(0, vol_l, vol_r);

        if (D_800E6B4C != 4 && D_800E6AB0 == 1) {
            func_800B7034(1, buf, D_800E6AF8);
            func_800B6FCC(1, D_800E6AF8);
            if (D_800E6AF8[0] != 0) {
                D_800E6AF0[0] = D_800E6AF8[0];
                D_800E6AF1 = D_800E6AF9;
                if (D_800E6AF0[0] >= 0x10) {
                    D_800E6B58 = 0xFF;
                }
            }
            if ((D_800E6AF8[0] & 0x40) != 0) {
                D_800E6B00++;
            } else {
                D_800E6B00 = 0;
                if ((D_800E6AF8[0] & 0x20) != 0) {
                    D_800E6AAC = 0;
                    D_800E6AB0 = 0;
                }
            }
            if (D_800E6B00 >= 0x2E) {
                if (D_800E6B4C == 3) {
                    D_800E6B4C = 0x5A;
                    D_800E6AB0 = 0;
                } else {
                    D_800E6B4C = 0;
                    D_800E6B00 = 0;
                    func_800AC074();
                }
            }
        }

        if (D_800E6BD8 == 1 || D_800E6ABA == 1) {
            if (D_800E6B4C != 4) {
                int next_vol = (signed short)(D_800E6AD6 - 0x10);
                D_800E6AD6 = (unsigned short)next_vol;
                if (next_vol <= 0) {
                    D_800E6AD6 = 0;
                    D_800E6ABA = 0;
                    D_800E6AB0 = 0;
                    D_800E6B4C = 0;
                    func_800A95E8();
                }
                D_800E6ADC = (signed short)D_800E6AD6;
                vol_l = (signed short)(((signed short)D_800E6AD6 * D_800E6B6C) / 128);
                func_800BCC58(0, vol_l, vol_l);
            }
        }

        if (D_800E6B4C == 1) {
            if (D_800E6AAC >= D_800E6AB4 && D_800E6BD8 == 0) {
                D_800E6B4C = 0;
                func_800AC074();
            }
        } else if (D_800E6B4C == 3) {
            if (D_800E6AAC >= D_800E6AB4) {
                D_800E6B4C = 0;
                D_800E6B58 = 0;
                if (B(D_800CE9D0, 1 + (D_800E6B76 - 0x300) * 12) != 0) {
                    D_800E6AB0 = 1;
                    func_800A95E8();
                } else {
                    func_800AC160();
                }
            }
        }
        return;

    case 5:
    {
        signed short next_vol = (signed short)(D_800E6AD6 - 8);
        D_800E6AD6 = (unsigned short)next_vol;
        if (next_vol <= 0) {
            D_800E6ADC = 0;
            D_800E6AD6 = 0;
            if (func_800B6FCC(1, 0) == 2) {
                func_800A95E8();
                D_800E6B4C = 0xA;
            } else {
                func_800ACD08();
            }
            return;
        }
        D_800E6ADC = next_vol;
        vol_l = (signed short)((next_vol * D_800E6B6C) / 128);
        func_800BCC58(0, vol_l, vol_l);
        return;
    }

    case 6:
        if (func_800B6FCC(1, 0) == 2) {
            func_800A95E8();
            D_800E6B4C = 7;
        } else {
            func_800ACD08();
        }
        return;

    case 7:
        if (func_800B6FCC(1, 0) == 2) {
            func_800AC074();
        } else {
            func_800ACD08();
        }
        return;

    case 8:
    {
        signed short cur_vol;
        func_800AB620(0x520);
        cur_vol = (signed short)D_800E6AD6;
        D_800E6B28 = 0;
        if (cur_vol <= 0) {
            D_800E6ADC = 0;
            D_800E6AD6 = 0;
            if (func_800B6FCC(1, D_800E6AF0) == 2) {
                func_800A95E8();
                D_800E6BB2 = 0;
                D_800E6B4C = 0x14;
            } else {
                func_800ACD08();
            }
            return;
        }
        cur_vol = (signed short)(cur_vol - 8);
        D_800E6AD6 = (unsigned short)cur_vol;
        D_800E6ADC = cur_vol;
        vol_l = (signed short)((cur_vol * D_800E6B6C) / 128);
        func_800BCC58(0, vol_l, vol_l);
        return;
    }

    case 10:
        if (func_800B6FCC(1, 0) == 2) {
            func_800AC074();
        } else {
            func_800ACC80();
        }
        return;

    case 20:
        if (func_800B6FCC(1, 0) == 2) {
            if (D_800E6B76 >= 0x300U) {
                func_800AC160();
            } else if (D_800E6B76 >= 0x200U) {
                func_800AC084();
            } else {
                D_800E6B4C = 0;
            }
        } else {
            func_800ACD08();
        }
        return;

    case 30:
        if (func_800B6FCC(1, 0) == 2) {
            D_800E6B00 = 0;
            func_800A95C0();
            D_800E6B4C = 0x28;
        } else {
            func_800ACD08();
        }
        return;

    case 40:
        if (func_800B6FCC(1, 0) == 2) {
            func_800A9568();
            vol_l = (signed short)(((signed short)D_800E6AD6 * D_800E6B6C) / 128);
            vol_r = (signed short)((D_800E6ADC * D_800E6B6C) / 128);
            D_800E6B4C = 3;
            func_800BCC58(0, vol_l, vol_r);
        } else {
            func_800ACD08();
        }
        return;

    case 60:
    case 64:
    {
        signed short next_vol = (signed short)(D_800E6AD6 - 12);
        D_800E6AD6 = (unsigned short)next_vol;
        if (next_vol < 13) {
            D_800E6ADC = 0;
            D_800E6AD6 = 0;
            if (func_800B6FCC(1, 0) == 2) {
                func_800A95E8();
                D_800E6B4C = (signed short)(D_800E6B4C + 1);
            } else {
                func_800ACC80();
            }
            return;
        }
        D_800E6ADC = next_vol;
        vol_l = (signed short)((next_vol * D_800E6B6C) / 128);
        func_800BCC58(0, vol_l, vol_l);
        return;
    }

    case 61:
    case 65:
        if (func_800B6FCC(1, 0) == 2) {
            func_800ABC34();
            if (D_800E6B4C == 0x45) {
                D_800E6B4C = 0x47;
            }
        }
        return;

    case 62:
    case 66:
    {
        signed short next_vol = (signed short)(D_800E6AD6 + 12);
        D_800E6AD6 = (unsigned short)next_vol;
        if (next_vol >= 0x75) {
            D_800E6ADC = 0x75;
            D_800E6AD6 = 0x75;
            D_800E6B4C = (D_800E6B4C == 0x3E) ? 3 : 4;
        } else {
            D_800E6ADC = next_vol;
        }
        vol_l = (signed short)(((signed short)D_800E6AD6 * D_800E6B6C) / 128);
        vol_r = (signed short)((D_800E6ADC * D_800E6B6C) / 128);
        func_800BCC58(0, vol_l, vol_r);
        return;
    }

    case 67:
        func_800A95E8();
        return;

    case 76:
        func_800AAAD8();
        return;

    case 90:
    {
        int res = -1;
        if (D_800E6B0C == 0x1E) {
            res = func_800B7034(1, 0, D_800E6AF0);
        }
        if (res == 1) {
            D_800E6BB2 = 0;
            D_800E6B4C = 0x5B;
        }
        return;
    }

    case 91:
        D_800E6BB2 = (unsigned short)(D_800E6BB2 + (unsigned short)D_8010CDDC);
        if (D_800E6BB2 >= 0xB5U) {
            D_800E6BB2 = 0;
            D_800E6BA0 = 0;
            D_800E6B4C = 0x5C;
        }
        return;

    case 92:
        if (func_800B6FCC(1, 0) == 2) {
            func_800B7034(1, 0, D_800E6AF0);
            D_800E6B4C = 0x5D;
        } else {
            func_800ACD08();
        }
        return;

    case 93:
        D_800E6BB2 = 0;
        D_800E6B00 = 0;
        D_800E6B4C = 8;
        return;

    default:
        return;
    }
}
