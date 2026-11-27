/*
 * func_800B8624 - Initialize SPU and CD-ROM XA stereo audio volume (CdInitVol)
 *
 * Purpose:
 *   Configures SPU control and CD audio input volumes through the register block
 *   pointed to by D_800D3C28 (if current volume +0x1B8 and +0x1BA are zero, sets
 *   main volume +0x180/+0x182 to 0x3FFF; always sets CD volume +0x1B0/+0x1B2 to
 *   0x3FFF and SPUCNT +0x1AA to 0xC001), then writes the CD-ROM audio attenuation
 *   matrix (ATV0=0x80, ATV1=0x00, ATV2=0x80, ATV3=0x00, ADPCTL=0x20) via
 *   D_800D3C14..D_800D3C20 (hardware ports 0x1F801800..0x1F801803).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   0 (int)
 *
 * Globals read:
 *   D_800D3C28 (u8 *): pointer to SPU register base (0x1F801C00)
 *     +0x180/+0x182 (u16): Main Volume L/R (0x1F801D80/2)
 *     +0x1AA        (u16): SPUCNT (0x1F801DAA) = 0xC001
 *     +0x1B0/+0x1B2 (u16): CD Audio Volume L/R (0x1F801DB0/2) = 0x3FFF
 *     +0x1B8/+0x1BA (u16): Current Main Volume L/R (0x1F801DB8/A)
 *   D_800D3C14 (volatile u8 *): CD-ROM Index/Status port (0x1F801800)
 *   D_800D3C18 (volatile u8 *): CD-ROM port +1 (0x1F801801)
 *   D_800D3C1C (volatile u8 *): CD-ROM port +2 (0x1F801802)
 *   D_800D3C20 (volatile u8 *): CD-ROM port +3 (0x1F801803)
 */

#define H(p, o) (*(unsigned short *)((p) + (o)))

extern unsigned char *D_800D3C28;
extern volatile unsigned char *D_800D3C14;
extern volatile unsigned char *D_800D3C18;
extern volatile unsigned char *D_800D3C1C;
extern volatile unsigned char *D_800D3C20;

int func_800B8624(void)
{
    unsigned char *spu = D_800D3C28;
    unsigned char mix[4];

    if (H(spu, 0x1B8) == 0 && H(spu, 0x1BA) == 0) {
        H(spu, 0x180) = 0x3FFFu;
        H(spu, 0x182) = 0x3FFFu;
        spu = D_800D3C28;
    }

    H(spu, 0x1B0) = 0x3FFFu;
    H(spu, 0x1B2) = 0x3FFFu;
    H(spu, 0x1AA) = 0xC001u;

    mix[0] = 0x80;
    mix[2] = 0x80;
    mix[1] = 0x00;
    mix[3] = 0x00;

    /* CD-ROM Register Bank 2: ATV0 (L->L) and ATV1 (L->R) */
    *D_800D3C14 = 2;
    *D_800D3C1C = mix[0];
    *D_800D3C20 = mix[1];

    /* CD-ROM Register Bank 3: ATV2 (R->R), ATV3 (R->L), ADPCTL=0x20 (apply) */
    *D_800D3C14 = 3;
    *D_800D3C18 = mix[2];
    *D_800D3C1C = mix[3];
    *D_800D3C20 = 0x20;

    return 0;
}
