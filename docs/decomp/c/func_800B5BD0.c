/*
 * func_800B5BD0
 *
 * Purpose:
 *   Initializes PS1 Geometry Transformation Engine (GTE / COP2), equivalent to
 *   PsyQ InitGeom(). Patches the BIOS C0 exception handler via func_800B5C78(),
 *   enables Coprocessor 2 (bit 30 = 0x40000000 in COP0 Status $12), and loads
 *   default projection/depth-cue/screen-offset GTE control registers:
 *     - ctc2 $29 (ZSF3) = 0x155 (341)
 *     - ctc2 $30 (ZSF4) = 0x100 (256)
 *     - ctc2 $26 (H)    = 0x3E8 (1000)
 *     - ctc2 $27 (DQA)  = -0x1062 (-4194)
 *     - ctc2 $28 (DQB)  = 0x01400000
 *     - ctc2 $24 (OFX)  = 0
 *     - ctc2 $25 (OFY)  = 0
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields written:
 *   D_800D3084 (unsigned int): saves/restores $ra without a stack frame
 */

extern unsigned int D_800D3084;

extern void func_800B5C78(void);

void func_800B5BD0(void)
{
    /* Assembly saves $ra in D_800D3084 across func_800B5C78() instead of stack */
    func_800B5C78();

    /*
     * COP0 Status ($12) |= 0x40000000 (CU2: enable GTE / Coprocessor 2)
     *
     * GTE control registers initialized:
     *   ZSF3 ($29) = 0x00000155
     *   ZSF4 ($30) = 0x00000100
     *   H    ($26) = 0x000003E8 (1000)
     *   DQA  ($27) = -0x1062    (-4194)
     *   DQB  ($28) = 0x01400000
     *   OFX  ($24) = 0
     *   OFY  ($25) = 0
     */
}
