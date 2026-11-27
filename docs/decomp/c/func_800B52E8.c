/*
 * func_800B52E8
 *
 * Purpose:
 *   Initializes 3D graphics/projection origin and mode state (equivalent to PsyQ
 *   GsInit3D). Sets the base screen origin D_800EF868[0..1] to half of the
 *   display dimensions (D_800FB57C / 2, D_800FB5AC / 2), calls func_800B4698()
 *   to update double-buffered VRAM draw/display offsets, and sets D_8010C29C = 10,
 *   D_8010C298 = 0 (light mode), and D_800FF704 = 0x3FFF.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800FB57C (s32): display/screen width
 *   D_800FB5AC (s32): display/screen height
 *
 * Global fields written:
 *   D_800EF868 (s16[2]): base screen X, Y origin (width / 2, height / 2)
 *   D_8010C29C (u32) = 10
 *   D_8010C298 (u32) = 0
 *   D_800FF704 (u32) = 0x3FFF
 */

extern signed int D_800FB57C;
extern signed int D_800FB5AC;
extern signed short D_800EF868[];
extern unsigned int D_8010C29C;
extern unsigned int D_8010C298;
extern unsigned int D_800FF704;

extern void func_800B4698(void);

void func_800B52E8(void)
{
    D_800EF868[0] = (signed short)(D_800FB57C / 2);
    D_800EF868[1] = (signed short)(D_800FB5AC / 2);

    func_800B4698();

    D_8010C29C = 10;
    D_8010C298 = 0;
    D_800FF704 = 0x3FFF;
}
