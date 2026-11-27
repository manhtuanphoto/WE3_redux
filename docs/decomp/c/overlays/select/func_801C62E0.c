/*
 * func_801C62E0 (overlay: select)
 *
 * Purpose:
 *   State machine for the trophy/records/gallery viewer sub-screen in the
 *   select overlay, dispatching on Scratchpad+0x29B (states 0, 1, 2, 3, 0xA,
 *   0x63):
 *     - State 0: initializes menu/scroll cursors (D_800AD638, D_801D8B15,
 *       D_801DA5D0..D_801DA5D6), 3D/camera parameters (D_8010A368..D_8010A38C),
 *       backdrop quads (D_801D8DA8), and sprite slots 5, 6, 0xC.
 *     - State 1: waits for fade-in via func_8006F07C(0x40).
 *     - State 2: polls pad input (func_8006EE60) for list navigation
 *       (Up/Down/Left/Right), item rotation/paging (D_801DA058 = +-1/+-10 ->
 *       state 3), confirm (Circle -> state 0xA), and cancel (Cross -> state 0x63).
 *     - State 3: steps D_801DA330 across 0..0x27 (wrapping and skipping 0x23)
 *       and updates the 3D/model view via func_801C98F8(0) until D_801DA058 == 0.
 *     - State 0xA: runs sub-dialog func_801C6B34() and transitions back to state 2.
 *     - State 0x63: fades out via func_8006F0DC(0x40, 1) and exits to state 0.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef signed short   s16;
typedef unsigned int   u32;
typedef signed int     s32;

/* PS1 Scratchpad RAM (0x1F800000..0x1F8003FF) */
#define SPAD_1EC (*(volatile u8 *)0x1F8001EC)
#define SPAD_29B (*(volatile u8 *)0x1F80029B)
#define SPAD_2A2 (*(volatile u8 *)0x1F8002A2)
#define SPAD_330 (*(volatile u8 *)0x1F800330)

typedef struct {
    s16 x;          /* 0x00 */
    s16 y;          /* 0x02 */
    s16 w;          /* 0x04 */
    s16 h;          /* 0x06 */
    u8  rgba[12];   /* 0x08..0x13 (0x801D8DB0..0x801D8DBB) */
} MenuBoxDesc;

extern u16 D_800AD638;      /* visible cursor row (0..10) */
extern u8  D_8010865C;      /* bitfield flag (bit 1 set when func_801C6B34() == 2) */
extern u8  D_8010A322;      /* outer select state machine index */
extern u8  D_8010A368;
extern u8  D_8010A369;
extern s16 D_8010A370;      /* camera/model X */
extern s16 D_8010A374;      /* camera/model Y */
extern s16 D_8010A378;      /* camera/model Z */
extern s16 D_8010A37C;
extern s16 D_8010A37E;
extern s16 D_8010A380;
extern s16 D_8010A38C;      /* scale (0x1000) */
extern s16 D_8010A5A4;      /* last pad input */
extern u8  D_801D8B15;      /* absolute selected item index (0..21) */
extern u8  D_801D8B17;
extern MenuBoxDesc D_801D8DA8;
extern s32 D_801DA058;      /* scroll/step delta for D_801DA330 */
extern s32 D_801DA060;
extern s32 D_801DA064;
extern u8  D_801DA068;
extern u8  D_801DA069;
extern u8  D_801DA06A;
extern u8  D_801DA06B;
extern u8  D_801DA2F0;
extern u8  D_801DA330;      /* displayed model/trophy index (0..0x27, skips 0x23) */
extern u8  D_801DA5C8;
extern s16 D_801DA5D0;
extern s16 D_801DA5D2;      /* list scroll offset */
extern s16 D_801DA5D4;
extern s16 D_801DA5D6;      /* visible page count (0xB = 11) */

extern void func_800171C4(s32 a0);
extern void func_800171DC(void);
extern void func_800171FC(s32 a0);
extern void func_8005C49C(void);
extern u32  func_8006EE60(s32 pad_port);
extern s32  func_8006F07C(s32 step);
extern s32  func_8006F0DC(s32 step, s32 mode);
extern void func_80096050(void);
extern void func_800AB620(s32 sfx_id);
extern void func_8018EC3C(s32 a0, s32 a1);
extern void func_80193FB4(s32 slot, s32 id, s32 a2, s32 a3,
                          s32 s0, s32 s1, s32 s2, s32 s3, s32 s4,
                          s32 s5, s32 s6, s32 s7, s32 s8, s32 s9,
                          s32 s10, s32 s11, s32 s12);
extern void func_801940EC(s32 slot, u32 flags, MenuBoxDesc *box, s32 a3, s32 s0, s32 s1);
extern s32  func_801C6B34(void);
extern void func_801C84E8(void);
extern void func_801C88BC(s32 a0, s32 a1);
extern void func_801C98F8(s32 init_flag);

void func_801C62E0(void)
{
    u8  state = SPAD_29B;
    u16 saved_cursor;
    u32 pad;
    u16 btn;

    switch (state) {
    case 0:
        saved_cursor = D_800AD638;
        D_801DA5D4 = 1;
        D_801DA5D6 = 0x0B;
        D_801D8B17 = 0;
        D_801DA5C8 = 1;
        D_801DA06B = 0;
        D_801DA069 = 0;
        D_801DA06A = 0;
        D_801DA068 = 0;
        D_801DA5D0 = 0;
        D_801DA5D2 = 0;
        SPAD_330   = 1;
        SPAD_2A2   = 0x81;
        SPAD_1EC   = 0;
        D_800AD638 = 0;
        D_801D8B15 = 0;
        D_801DA060 = 0;
        D_801DA058 = 0;
        D_8010A5A4 = 0;
        D_801DA064 = saved_cursor;

        func_801C98F8(1);
        func_80096050();
        func_8005C49C();

        D_8010A370 = -0x134;
        D_8010A374 = -0xC4;
        D_8010A378 = 0x76C;
        D_8010A368 = 1;
        D_8010A380 = 0;
        D_8010A37E = 0;
        D_8010A37C = 0;
        D_8010A38C = 0x1000;
        D_8010A369 = 0;

        func_80193FB4(5, 0x9001, 0, 0,
                      0, 0x16, 0,
                      0x1000, 0x1000, 0, 0,
                      0x80, 0x80, 0x80, 0, 2, 1);
        func_80193FB4(6, 0x9002, 0, 0,
                      0, 0x16, 0,
                      0x1000, 0x1000, 0, 0,
                      0x80, 0x80, 0x80, 0, 2, 1);

        func_801C88BC(0, 0);

        D_801D8DA8.x = -0x43;
        D_801D8DA8.y = -0x47;
        D_801D8DA8.w = 0xE9;
        D_801D8DA8.h = 0x7B;
        {
            s32 i;
            for (i = 0; i < 12; i++) {
                D_801D8DA8.rgba[i] = 0;
            }
        }
        func_801940EC(2, 0x40000000U, &D_801D8DA8, 0, 5, 1);

        D_801D8DA8.y = 0x3D;
        D_801D8DA8.h = 0x28;
        func_801940EC(1, 0x40000000U, &D_801D8DA8, 0, 5, 1);

        func_80193FB4(0x0C, 0x9004, 0, 0,
                      0, 9, 0,
                      0x1000, 0x1000, 0, 0,
                      0x80, 0x80, 0x80, 0, 6, 1);
        func_800171DC();
        break;

    case 1:
        if (func_8006F07C(0x40) != 0) {
            func_800171DC();
        }
        break;

    case 2:
        pad = func_8006EE60(0);
        D_8010A5A4 = (s16)pad;
        btn = (u16)(pad & 0xFFFF);

        switch (btn) {
        case 0x0001:
            D_801DA058 = -10;
            SPAD_29B++;
            break;
        case 0x0002:
            D_801DA058 = 10;
            SPAD_29B++;
            break;
        case 0x0008:
            D_801DA058 = 1;
            SPAD_29B++;
            break;
        case 0x0004:
            D_801DA058 = -1;
            SPAD_29B++;
            break;
        case 0x1000: /* D-Pad Up */
            if (D_800AD638 != 0) {
                D_800AD638--;
                D_801D8B15--;
                func_800AB620(0x50D);
            } else if (D_801DA5D2 != 0) {
                D_801DA5D2--;
                D_801D8B15--;
                func_800AB620(0x50D);
            }
            break;
        case 0x4000: /* D-Pad Down */
            if (D_800AD638 < 0x0A) {
                D_800AD638++;
                D_801D8B15++;
                func_800AB620(0x50D);
            } else if (D_801DA5D2 < (0x16 - D_801DA5D6)) {
                D_801DA5D2++;
                D_801D8B15++;
                func_800AB620(0x50D);
            }
            break;
        case 0x2000: /* D-Pad Right */
            if (D_801D8B15 < 0x0B) {
                D_800AD638 = 0x0A;
                D_801D8B15 = 0x15;
                D_801DA5D2 = 0x0B;
                func_800AB620(0x50D);
            }
            break;
        case 0x8000: /* D-Pad Left */
            if (D_801DA5D2 != 0) {
                D_801DA5D2 = 0;
                D_801D8B15 = 0;
                D_800AD638 = 0;
                func_800AB620(0x50D);
            }
            break;
        case 0x0020: /* Circle (Confirm) */
            func_800AB620(0x528);
            D_8010A322 = 0;
            func_800171FC(0x0A);
            break;
        case 0x0040: /* Cross (Cancel) */
            func_800AB620(0x529);
            func_800171FC(0x63);
            break;
        default:
            break;
        }
        func_801C84E8();
        break;

    case 3:
        if (D_801DA330 == 0 && D_801DA058 < 0) {
            D_801DA330 = 0x27;
        } else if (D_801DA330 >= 0x27 && D_801DA058 > 0) {
            D_801DA330 = 0;
        } else if (D_801DA058 < 0) {
            D_801DA330--;
        } else {
            D_801DA330++;
        }

        if (D_801DA330 == 0x23) {
            D_801DA330 = (D_801DA058 < 0) ? 0x22 : 0x24;
        }

        func_801C98F8(0);

        if (D_801DA058 != 0) {
            if (D_801DA058 < 0) {
                D_801DA058++;
            } else {
                D_801DA058--;
            }
        }
        if (D_801DA058 == 0) {
            SPAD_29B--;
        }
        break;

    case 0x0A:
        {
            u8 res = (u8)func_801C6B34();
            if (res == 1 || res == 2) {
                if (res == 2) {
                    D_8010865C |= 2;
                }
                func_801C88BC(0, 0);
                D_801D8B17 = 0;
                func_8018EC3C(0x1F, 0);
                func_801C84E8();
                func_800171FC(2);
            }
        }
        break;

    case 0x63:
        if (func_8006F0DC(0x40, 1) != 0) {
            SPAD_2A2 = 0;
            SPAD_330 = 0;
            func_800171C4(0x10);
            func_800171FC(0);
            D_8010A322 = 0;
        }
        break;

    default:
        break;
    }
}
