/*
 * func_80022AAC - Main player actor per-frame update & state dispatcher.
 *
 * Purpose:
 *   Reads player control mode via func_800228BC (human P1/P2, AI, replay/frozen)
 *   to populate pad inputs (+0x3D4/+0x3D6) and control flags (+0x39A/+0x39C/+0x3E1),
 *   then dispatches to the handler for actor+0x396 (states 0..62, 254, and default)
 *   and decrements the +0x3D0 timer.
 *
 * Arguments:
 *   actor (unsigned char *): player actor struct (1000 bytes).
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F800230 / 0x1F800234 (u16, R): P1 pad buttons / edge or stick inputs.
 *   - 0x1F800238 / 0x1F80023C (u16, R): P2 pad buttons / edge or stick inputs.
 *   - 0x1F80028C (u8, W): high nibble / rotated pad direction bits.
 *   - 0x1F8002E7 (u8, R): camera angle / pad orientation mode (0, 0x80, other).
 *
 * Actor fields reads/writes:
 *   - actor: +0x02/+0x06 (s16 x/z, R), +0x396 (u8 state, R),
 *            +0x39A (u8 control mode: 0=human, 2=AI, W),
 *            +0x39C (u8 active input flag, W),
 *            +0x3BC/+0x3BE (s16 saved x/z, W),
 *            +0x3D0 (u8 timer, R/W: decremented if > 0),
 *            +0x3D4/+0x3D6 (u16 pad input / triggers, R/W),
 *            +0x3D8 (u16 saved pad input, R/W),
 *            +0x3E1 (u8 flag, W=0).
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned int func_800228BC(unsigned char *actor);
extern void func_8006C3D8(unsigned char *actor);
extern void func_8005CE4C(unsigned char *actor);

extern void func_80023078(unsigned char *actor);
extern void func_800243D8(unsigned char *actor);
extern void func_80025268(unsigned char *actor);
extern void func_800259F0(unsigned char *actor);
extern void func_80029074(unsigned char *actor);
extern void func_8002BC78(unsigned char *actor);
extern void func_80023A78(unsigned char *actor);
extern void func_800318AC(unsigned char *actor);
extern void func_80033EEC(unsigned char *actor);
extern void func_800332EC(unsigned char *actor);
extern void func_8002F70C(unsigned char *actor);
extern void func_8003E204(unsigned char *actor);
extern void func_80040324(unsigned char *actor);
extern void func_800264CC(unsigned char *actor);
extern void func_80041898(unsigned char *actor);
extern void func_800267D8(unsigned char *actor);
extern void func_80026A64(unsigned char *actor);
extern void func_80026B3C(unsigned char *actor);
extern void func_80026D14(unsigned char *actor);
extern void func_80035184(unsigned char *actor);
extern void func_800420E4(unsigned char *actor);
extern void func_800442C4(unsigned char *actor);
extern void func_80027140(unsigned char *actor);
extern void func_80044E40(unsigned char *actor);
extern void func_80044BB8(unsigned char *actor);
extern void func_800273F4(unsigned char *actor);
extern void func_80028F98(unsigned char *actor);
extern void func_8003C0E8(unsigned char *actor);
extern void func_8003A8E0(unsigned char *actor);
extern void func_8003A174(unsigned char *actor);
extern void func_8003B8AC(unsigned char *actor);
extern void func_80035DE4(unsigned char *actor);
extern void func_800367E8(unsigned char *actor);
extern void func_80036C60(unsigned char *actor);
extern void func_800383F8(unsigned char *actor);
extern void func_8003B0C0(unsigned char *actor);
extern void func_8003C46C(unsigned char *actor);
extern void func_80076A4C(unsigned char *actor);
extern void func_8007AD20(unsigned char *actor);
extern void func_800842A4(unsigned char *actor);
extern void func_80085C68(unsigned char *actor);
extern void func_8007B700(unsigned char *actor);
extern void func_80077D8C(unsigned char *actor);
extern void func_8007A154(unsigned char *actor);
extern void func_8007C9B0(unsigned char *actor);
extern void func_8007DD14(unsigned char *actor);
extern void func_8007950C(unsigned char *actor);
extern void func_800828C8(unsigned char *actor);
extern void func_801D2DA4(unsigned char *actor);
extern void func_801D4244(unsigned char *actor);
extern void func_801D30A0(unsigned char *actor);
extern void func_801D31C8(unsigned char *actor);
extern void func_801D3500(unsigned char *actor);
extern void func_801D3C50(unsigned char *actor);
extern void func_801D3F08(unsigned char *actor);
extern void func_801D4FA8(unsigned char *actor);
extern void func_80189894(unsigned char *actor);
extern void func_8008A240(unsigned char *actor);

void func_80022AAC(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned int mode = func_800228BC(actor);

    if (mode < 6U) {
        switch (mode) {
        case 0:
            H(actor, 0x3D6) = 0;
            H(actor, 0x3D4) = 0;
            B(scratch, 0x28C) = 0;
            func_8006C3D8(actor);
            B(actor, 0x39C) = 1;
            break;

        case 1:
            H(actor, 0x3D4) = H(scratch, 0x230);
            H(actor, 0x3D6) = H(scratch, 0x234);
            goto decode_pad_dir;

        case 2:
            H(actor, 0x3D4) = H(scratch, 0x238);
            H(actor, 0x3D6) = H(scratch, 0x23C);
decode_pad_dir:
        {
            unsigned int cam_mode = B(scratch, 0x2E7);
            unsigned int pad = H(actor, 0x3D4);

            if (cam_mode == 0) {
                B(scratch, 0x28C) = (unsigned char)(((pad & 0x7000) >> 11) | (pad >> 15));
            } else if (cam_mode == 0x80) {
                B(scratch, 0x28C) = (unsigned char)((pad >> 13) | ((pad & 0x1000) >> 9));
            } else {
                B(scratch, 0x28C) = (unsigned char)(pad >> 12);
            }
            B(actor, 0x39A) = 0;
            B(actor, 0x39C) = 1;
            B(actor, 0x3E1) = 0;
            H(actor, 0x3BC) = H(actor, 2);
            H(actor, 0x3BE) = H(actor, 6);
            break;
        }

        case 3:
            func_8005CE4C(actor);
            B(actor, 0x39A) = 2;
            B(scratch, 0x28C) = (unsigned char)(H(actor, 0x3D4) >> 12);
            if (B(actor, 0x396) != 7) {
                H(actor, 0x3BC) = H(actor, 2);
                H(actor, 0x3BE) = H(actor, 6);
            }
            B(actor, 0x39C) = 1;
            B(actor, 0x3E1) = 0;
            break;

        case 4:
            func_8005CE4C(actor);
            B(actor, 0x39C) = 1;
            H(actor, 0x3D8) = H(actor, 0x3D4);
            B(scratch, 0x28C) = (unsigned char)(H(actor, 0x3D4) >> 12);
            break;

        case 5:
            B(actor, 0x39C) = 0;
            H(actor, 0x3D4) = H(actor, 0x3D8);
            break;
        }
    }

    switch (B(actor, 0x396)) {
    case 0:  func_80023078(actor); break;
    case 1:  func_800243D8(actor); break;
    case 2:
    case 19: func_80025268(actor); break;
    case 3:  func_800259F0(actor); break;
    case 4:  func_80029074(actor); break;
    case 5:
    case 6:  func_8002BC78(actor); break;
    case 7:  func_80023A78(actor); break;
    case 8:  func_800318AC(actor); break;
    case 9:  func_80033EEC(actor); break;
    case 10: func_800332EC(actor); break;
    case 11: func_8002F70C(actor); break;
    case 12: func_8003E204(actor); break;
    case 13: func_80040324(actor); break;
    case 14: func_800264CC(actor); break;
    case 15: func_80041898(actor); break;
    case 16: func_800267D8(actor); break;
    case 17: func_80026A64(actor); break;
    case 18: func_80026B3C(actor); break;
    case 20: func_80026D14(actor); break;
    case 21: func_80035184(actor); break;
    case 22: func_800420E4(actor); break;
    case 23: func_800442C4(actor); break;
    case 24: func_80027140(actor); break;
    case 25: func_80044E40(actor); break;
    case 26: func_80044BB8(actor); break;
    case 27: func_800273F4(actor); break;
    case 29: func_80028F98(actor); break;
    case 30: func_8003C0E8(actor); break;
    case 31: func_8003A8E0(actor); break;
    case 32: func_8003A174(actor); break;
    case 33: func_8003B8AC(actor); break;
    case 34: func_80035DE4(actor); break;
    case 35: func_800367E8(actor); break;
    case 36:
    case 37: func_80036C60(actor); break;
    case 38:
    case 39:
    case 40: func_800383F8(actor); break;
    case 41: func_8003B0C0(actor); break;
    case 42: func_8003C46C(actor); break;
    case 43: func_80076A4C(actor); break;
    case 44: func_8007AD20(actor); break;
    case 45: func_800842A4(actor); break;
    case 46: func_80085C68(actor); break;
    case 47: func_8007B700(actor); break;
    case 48: func_80077D8C(actor); break;
    case 49: func_8007A154(actor); break;
    case 50: func_8007C9B0(actor); break;
    case 51: func_8007DD14(actor); break;
    case 53: func_8007950C(actor); break;
    case 54: func_800828C8(actor); break;
    case 55: func_801D2DA4(actor); break;
    case 56: func_801D4244(actor); break;
    case 57: func_801D30A0(actor); break;
    case 58: func_801D31C8(actor); break;
    case 59: func_801D3500(actor); break;
    case 60: func_801D3C50(actor); break;
    case 61: func_801D3F08(actor); break;
    case 62: func_801D4FA8(actor); break;
    case 254: func_80189894(actor); break;
    default: func_8008A240(actor); break;
    }

    if (B(actor, 0x3D0) != 0) {
        B(actor, 0x3D0)--;
    }
}
