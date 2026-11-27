/*
 * func_800828C8
 *
 * Purpose:
 *   Penalty kick taker state machine (covers both in-match PKs and PK shootouts):
 *   handles pre-kick positioning and referee whistle, CPU/player directional aim
 *   and power/chip selection, run-up, kick execution with ball spin & stats recording,
 *   PK shootout result evaluation (scored/saved) with scoreboard update, and
 *   post-kick celebration / despair animations.
 *
 * Arguments:
 *   actor (u8 *): pointer to 1000-byte actor struct (the penalty kicker).
 *
 * Return value:
 *   void
 *
 * Scratchpad / Actor fields read & written:
 *   Scratchpad:
 *     0x12 + side*2 (u16): controller button mask for player side
 *     0x36/0x66 (s16): ball placement x/z on kick frame
 *     0xE0 (u16): flag checked during shootout outcome wait
 *     0xF8/0xFC (s16): predicted target x/z (cleared to 0 in substate 0x30)
 *     0x1E8 (u8): replay/event active flag (written 1 on kick, 0 on finish)
 *     0x1F0..0x20F / 0x210..0x22F (32 bytes): match camera/replay buffer copied on kick
 *     0x294 (u16): phase frame timer
 *     0x296 (s16/u16): pre-whistle delay timer (0x5A = 90 frames, then 0xFF)
 *     0x29A (u8): match phase (2 = in-match PK, 8 = PK shootout)
 *     0x2B0 (u32): ball event code (written 9 on kick; checked 0xB / 0xD)
 *     0x2B4/0x2B6 (s16): shot/event target coordinates
 *     0x2C0 (u8): written 1 when run-up starts
 *     0x2DB/0x2DC (u8): team scores (passed to func_80083D94)
 *     0x2E2 (u8): mode flag in substate 1
 *     0x2F0/0x2F2 (u8): flags written 0 / 3 on kick
 *     0x2F4 (u8 *): pointer to ball actor
 *     0x309 (u8): ready flag checked in substate 1
 *   Actor:
 *     +0x02/+0x06 (s16): x/z position
 *     +0x08/+0x10 (s32): x/z velocity (written in substate 0x52)
 *     +0x38E (u16): current anim id
 *     +0x390 (u8): anim frame
 *     +0x392 (u8): team slot (0..10)
 *     +0x396 (u8): state (writes 0 or 0x33)
 *     +0x397 (u8): substate (0..7, 0x30, 0x40, 0x50..0x52, 0x60..0x61, 0x68, 0x70, 0x78, 0x80, 0x88, 0x90, 0x98)
 *     +0x398 (u8): side (0/1)
 *     +0x399 (u8): attack direction flag
 *     +0x39A (u8): CPU control flag
 *     +0x3A0 (u32): speed
 *     +0x3A4 (u8): move heading
 *     +0x3A7 (u8): active flag (written 1)
 *     +0x3AA (u8): body heading
 *     +0x3AB (u8): target heading
 *     +0x3AC (u8): mirror / foot flag
 *     +0x3B0 (u8): cleared to 0 or set to 2
 *     +0x3C2 (s16): kick type flag (0 = standard PK, 1 = chipped/panenka PK)
 *     +0x3C4 (s16): celebration target z
 *     +0x3C6 (s16): kick lift parameter
 *     +0x3D4 (u16): buffered PK direction/input bits (masked with 0xE000)
 *     +0x3D6 (u16): kick trigger button mask
 *
 * Units & magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x5A (90 frames)  = 3.0 s pre-whistle wait
 *   0x1E (30 frames)  = 1.0 s CPU decision delay
 *   0x12D (301 frames)= 10.03 s kick timeout
 *   0xC (12 frames)   = 0.4 s minimum wait after shootout kick
 *   0x3C (60 frames)  = 2.0 s shootout ball-check window
 *   0x3320 (13088)    = ~52.35 m (goal line x)
 *   0x332 (818)       = ~3.27 m, +0..255 = ~3.27..4.29 m (wide corner aim z)
 *   0x280 (640)       = ~2.56 m, +-0x40 (64 = ~0.26 m) (inside-post aim z)
 *   0x433 (1075)      = ~4.30 m (shootout ball z check range)
 *   0xBC (188), 0xB4 (180) = kick power << 8 passed to func_8004B0F0
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800CA5C4[];
extern unsigned char D_800E6D60[];
extern unsigned int D_800E7658[];
extern unsigned char D_800E7DE9[];
extern unsigned char D_800F0EA1[];
extern unsigned char D_800F3566[];
extern unsigned char D_800FF748[];
extern unsigned char D_8010A400[];
extern unsigned char D_8010C32C[];

extern unsigned char *func_8001D004(int idx);
extern void func_800171DC(void);
extern unsigned int func_8001E5A4(unsigned int angle, int scale);
extern unsigned int func_8001E694(unsigned int angle, int scale);
extern int func_8001E6B8(int n);
extern void func_800479B8(unsigned char *actor, int a1);
extern void func_80049A94(unsigned char *actor, int a1, int a2, int a3, int a4);
extern void func_8004A0B0(unsigned char *actor, int a1);
extern void func_8004B0F0(unsigned char *actor, unsigned int heading, unsigned int power, int lift);
extern void func_80051A84(unsigned char *buf);
extern void func_80059250(unsigned char *actor);
extern void func_80059420(unsigned char *actor, int scored);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_8005B61C(int x1, int z1, int x2, int z2);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);
extern void func_8005B908(unsigned char *actor);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005BD1C(unsigned char *actor);
extern int func_8005C7A0(int n);
extern void func_8005C7EC(unsigned char *actor);
extern void func_80076D44(void);
extern void func_80079BA0(unsigned char *actor);
extern unsigned int func_80083D94(unsigned int score0, unsigned int score1);
extern unsigned int func_80083E88(unsigned char *actor);
extern void func_8009574C(void);
extern void func_800AB620(int sfx_id);

void func_800828C8(unsigned char *actor)
{
    unsigned char *scratch = Scratchpad;
    unsigned char substate = B(actor, 0x397);

    if (substate >= 0x99) {
        func_8005B908(actor);
        return;
    }

    switch (substate) {
    case 0:
        B(actor, 0x3A7) = 1;
        W(actor, 0x3A0) = 0;
        break;

    case 1:
        B(actor, 0x3A7) = 1;
        if (SW(D_800E7658, 0) >= 0x10 && B(scratch, 0x309) != 0) {
            unsigned char mode;
            func_80059250(actor);
            H(scratch, 0x294) = 0;
            H(scratch, 0x296) = 0;
            mode = B(scratch, 0x2E2);
            if (mode == 2 || mode == 3) {
                int idx = B(D_8010A400, 0) & 1;
                B(D_800FF748 + 0x65E0 + idx * 40, 4) = 1;
            } else if (mode == 1) {
                B(D_800FF748, 0x660C) = 1;
            } else {
                B(D_800FF748, 0x65E4) = 1;
            }
            B(actor, 0x397)++;
        }
        break;

    case 2: {
        signed short t296;
        signed short dir_sign;
        int target_x;
        int z_off;
        unsigned int stat_word;
        unsigned int stat_nibble;
        unsigned char *ball;

        B(actor, 0x3A7) = 1;
        t296 = S(scratch, 0x296);
        if (t296 < 0x5A) {
            H(scratch, 0x296) = (unsigned short)(t296 + 1);
            H(scratch, 0x294) = 0;
            break;
        }
        if (t296 != 0xFF) {
            H(scratch, 0x296) = 0xFF;
            func_800AB620(0x516);
            break;
        }

        H(scratch, 0x294)++;
        func_8005C7EC(actor);

        if (B(actor, 0x39A) != 0 && H(scratch, 0x294) >= 0x1E) {
            switch (func_8001E6B8(10)) {
            case 0:
            case 1:
                H(actor, 0x3D4) = 0x6000;
                break;
            case 2:
            case 3:
                H(actor, 0x3D4) = 0x2000;
                break;
            case 4:
            case 5:
                H(actor, 0x3D4) = 0xC000;
                break;
            case 6:
            case 7:
                H(actor, 0x3D4) = 0x8000;
                break;
            case 8:
                H(actor, 0x3D4) = 0x4000;
                break;
            case 9:
                H(actor, 0x3D4) = 0;
                break;
            }
            H(actor, 0x3D6) = H(scratch, 0x12 + B(actor, 0x398) * 2);
        }

        if ((H(scratch, 0x12 + B(actor, 0x398) * 2) & H(actor, 0x3D6)) == 0 &&
            H(scratch, 0x294) < 0x12D) {
            break;
        }

        if (B(scratch, 0x29A) == 8 || B(actor, 0x399) != 0) {
            dir_sign = -1;
        } else {
            dir_sign = 1;
        }

        target_x = 0x3320 * dir_sign;
        switch (H(actor, 0x3D4) & 0xE000) {
        case 0x6000:
            B(D_800F0EA1, 0) = 5;
            if (func_80083E88(actor) & 0xFF) {
                z_off = (-0x332 - func_8001E6B8(0x100)) * dir_sign;
            } else {
                z_off = (-((signed short)func_8005C7A0(0x40) + 0x280)) * dir_sign;
            }
            H(actor, 0x3C6) = 3;
            H(actor, 0x3C2) = 1;
            break;

        case 0xC000:
            B(D_800F0EA1, 0) = 3;
            if (func_80083E88(actor) & 0xFF) {
                z_off = (func_8001E6B8(0x100) + 0x332) * dir_sign;
            } else {
                z_off = ((signed short)func_8005C7A0(0x40) + 0x280) * dir_sign;
            }
            H(actor, 0x3C6) = 3;
            H(actor, 0x3C2) = 1;
            break;

        case 0x8000:
            B(D_800F0EA1, 0) = 0;
            z_off = ((signed short)func_8005C7A0(0x40) + 0x280) * dir_sign;
            H(actor, 0x3C6) = 0x12;
            if (func_80083E88(actor) & 0xFF) {
                if (func_8001E6B8(2) != 0) {
                    H(actor, 0x3C6) = (unsigned short)(func_8001E6B8(4) + 0x14);
                } else {
                    z_off = (func_8001E6B8(0x100) + 0x332) * dir_sign;
                }
            }
            H(actor, 0x3C2) = 0;
            break;

        case 0x2000:
            B(D_800F0EA1, 0) = 2;
            z_off = (-((signed short)func_8005C7A0(0x40) + 0x280)) * dir_sign;
            H(actor, 0x3C6) = 0x12;
            if (func_80083E88(actor) & 0xFF) {
                if (func_8001E6B8(2) != 0) {
                    H(actor, 0x3C6) = (unsigned short)(func_8001E6B8(4) + 0x14);
                } else {
                    z_off = (-0x332 - func_8001E6B8(0x100)) * dir_sign;
                }
            }
            H(actor, 0x3C2) = 0;
            break;

        case 0x4000:
            B(D_800F0EA1, 0) = 4;
            z_off = func_8005C7A0(0x80);
            H(actor, 0x3C6) = 3;
            H(actor, 0x3C2) = 1;
            break;

        default:
            B(D_800F0EA1, 0) = 1;
            z_off = func_8005C7A0(0x60);
            if (func_80083E88(actor) & 0xFF) {
                H(actor, 0x3C6) = (unsigned short)(func_8001E6B8(4) + 0x18);
            } else {
                H(actor, 0x3C6) = 0x12;
            }
            H(actor, 0x3C2) = 0;
            break;
        }

        stat_word = W(D_8010C32C, B(actor, 0x392) * 12 + B(actor, 0x398) * 264);
        stat_nibble = (stat_word >> 4) & 0xF;
        H(actor, 0x3C6) += (unsigned short)func_8001E6B8((int)(9 - stat_nibble) >> 1);

        ball = (unsigned char *)W(scratch, 0x2F4);
        B(actor, 0x3AB) = (unsigned char)func_8005B61C(S(ball, 0x2), S(ball, 0x6), (signed short)target_x, (signed short)z_off);

        if (B(scratch, 0x29A) != 8) {
            unsigned int stat_b = B(D_8010C32C, B(actor, 0x392) * 12 + B(actor, 0x398) * 264) >> 4;
            B(actor, 0x3AB) += (unsigned char)func_8005C7A0(10 - stat_b);
        }

        B(scratch, 0x2C0) = 1;
        func_8005BA30(actor, 0xF, 0);
        H(scratch, 0x294) = 0;
        if (S(actor, 0x3C2) == 0) {
            B(actor, 0x397) = 3;
        } else {
            unsigned char heading = B(actor, 0x3AA);
            heading = B(actor, 0x3AC) ? (unsigned char)(heading + 4) : (unsigned char)(heading - 4);
            B(actor, 0x3AA) = heading;
            B(actor, 0x3A4) = heading;
            B(actor, 0x397) = 5;
        }
        break;
    }

    case 3:
        B(actor, 0x3A7) = 1;
        func_8005BD1C(actor);
        func_80079BA0(actor);
        if (B(actor, 0x390) == 0x1A) {
            unsigned char *ball;
            unsigned char *ball2;
            int r;
            unsigned int diff;
            int spin;

            B(actor, 0x3AA) = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xA);
            func_8004B0F0(actor, B(actor, 0x3AB), 0xBC, S(actor, 0x3C6));

            ball = (unsigned char *)W(scratch, 0x2F4);
            H(ball, 0x3CA) = (unsigned short)(((B(ball, 0x3A4) - B(actor, 0x3AA)) & 0xFF) < 0x81U);

            ball2 = (unsigned char *)W(scratch, 0x2F4);
            r = func_8001E6B8(2);
            ball = (unsigned char *)W(scratch, 0x2F4);
            diff = (B(actor, 0x3AA) - B(ball, 0x3A4)) & 0xFF;
            if (diff < 0x81U) {
                spin = (int)diff >> 6;
            } else {
                spin = (int)((B(ball, 0x3A4) - B(actor, 0x3AA)) & 0xFF) >> 6;
            }
            H(ball2, 0x3C8) = (unsigned short)((0xC - r) - spin);

            ball = (unsigned char *)W(scratch, 0x2F4);
            H(ball, 0x2) = H(scratch, 0x36);
            ball = (unsigned char *)W(scratch, 0x2F4);
            H(ball, 0x6) = H(scratch, 0x66);
            ball = (unsigned char *)W(scratch, 0x2F4);
            B(ball, 0x396) = 4;

            W(scratch, 0x2B0) = 9;
            func_8009574C();
            func_800AB620(0x507);
            B(actor, 0x397)++;

            if (B(scratch, 0x29A) != 8) {
                int i;
                func_80049A94(actor, 2, 0, S(scratch, 0x2B4), S(scratch, 0x2B6));
                B(scratch, 0x2F2) = 3;
                func_80051A84(scratch + 0x1F0);
                for (i = 0; i < 32; i += 4) {
                    W(scratch, 0x210 + i) = W(scratch, 0x1F0 + i);
                }
                B(D_800FF748, 0x8EC8 + B(actor, 0x398))++;
                B(D_800FF748, 0x88D1 + B(actor, 0x398) * 22 + B(actor, 0x392))++;
                func_80076D44();
                B(scratch, 0x1E8) = 1;
                for (i = 0; i < 32; i += 4) {
                    W(scratch, 0x210 + i) = W(scratch, 0x1F0 + i);
                }
                B(scratch, 0x2F0) = 0;
                B(scratch, 0x2F2) = 0;
            }
        }
        break;

    case 4:
        B(actor, 0x3A7) = 1;
        func_8005BD1C(actor);
        func_80079BA0(actor);
        if (B(actor, 0x390) == 0x2B) {
            if (B(scratch, 0x29A) == 8) {
                B(actor, 0x397) = 7;
                B(actor, 0x3B0) = 0;
                H(scratch, 0x294) = 0;
            } else {
                B(scratch, 0x1E8) = 0;
                B(actor, 0x396) = 0;
                B(actor, 0x397) = 0;
            }
        }
        break;

    case 5:
        func_8005BD1C(actor);
        func_80079BA0(actor);
        if (B(actor, 0x390) == 0x10) {
            func_8005BA30(actor, 0x10, 0);
            B(actor, 0x397)++;
        }
        break;

    case 6:
        func_8005BD1C(actor);
        if (B(actor, 0x390) < 0xB) {
            func_8004A0B0(actor, 2);
        }
        if (B(actor, 0x390) == 9) {
            unsigned char *ball;
            unsigned char *ball2;
            int r;
            unsigned int diff;
            int spin;

            func_8004B0F0(actor, B(actor, 0x3AB), 0xB4, S(actor, 0x3C6));

            ball = (unsigned char *)W(scratch, 0x2F4);
            H(ball, 0x3CA) = (unsigned short)(((B(ball, 0x3A4) - B(actor, 0x3AA)) & 0xFF) < 0x81U);

            ball2 = (unsigned char *)W(scratch, 0x2F4);
            r = func_8001E6B8(2);
            ball = (unsigned char *)W(scratch, 0x2F4);
            diff = (B(actor, 0x3AA) - B(ball, 0x3A4)) & 0xFF;
            if (diff < 0x81U) {
                spin = (int)diff >> 6;
            } else {
                spin = (int)((B(ball, 0x3A4) - B(actor, 0x3AA)) & 0xFF) >> 6;
            }
            H(ball2, 0x3C8) = (unsigned short)((0xC - r) - spin);

            ball = (unsigned char *)W(scratch, 0x2F4);
            B(ball, 0x396) = 4;
            W(scratch, 0x2B0) = 9;
            func_8009574C();
            func_800AB620(0x507);

            B(actor, 0x3AB) = B((unsigned char *)W(scratch, 0x2F4), 0x3A4);
            if (B(scratch, 0x29A) == 2) {
                int i;
                func_80049A94(actor, 2, 0, S(scratch, 0x2B4), S(scratch, 0x2B6));
                B(scratch, 0x2F2) = 3;
                func_80051A84(scratch + 0x1F0);
                for (i = 0; i < 32; i += 4) {
                    W(scratch, 0x210 + i) = W(scratch, 0x1F0 + i);
                }
                B(D_800FF748, 0x8EC8 + B(actor, 0x398))++;
                B(D_800FF748, 0x88D1 + B(actor, 0x398) * 22 + B(actor, 0x392))++;
                func_80076D44();
                B(scratch, 0x1E8) = 1;
                for (i = 0; i < 32; i += 4) {
                    W(scratch, 0x210 + i) = W(scratch, 0x1F0 + i);
                }
                B(scratch, 0x2F0) = 0;
                B(scratch, 0x2F2) = 0;
            }
        }
        if (B(actor, 0x390) == 0x16) {
            if (B(scratch, 0x29A) == 8) {
                B(actor, 0x397) = 7;
                B(actor, 0x3B0) = 0;
                H(scratch, 0x294) = 0;
            } else {
                B(scratch, 0x1E8) = 0;
                B(actor, 0x396) = 0;
                B(actor, 0x397) = 0;
                return;
            }
        }
        W(actor, 0x3A0) = B(D_800CA5C4, B(actor, 0x390));
        break;

    case 7: {
        unsigned int spd;
        unsigned int t294;
        unsigned char *pk_tbl;
        unsigned char kick_num;
        unsigned int slot_idx;
        int r;
        unsigned char next_sub;

        B(actor, 0x3A7) = 1;
        spd = W(actor, 0x3A0);
        W(actor, 0x3A0) = (spd < 0x13U) ? 0 : (spd - 0x12);

        H(scratch, 0x294)++;
        t294 = H(scratch, 0x294);
        if (t294 < 0xC) {
            break;
        }
        if (t294 < 0x3C) {
            unsigned int ang = func_8005B36C(S(actor, 0x2), S(actor, 0x6), 0, 0);
            unsigned char ball_dir = B((unsigned char *)W(scratch, 0x2F4), 0x3A4);
            unsigned int diff = (ang - ball_dir) & 0xFF;
            int ang_far;
            if (diff < 0x81U) {
                ang = func_8005B36C(S(actor, 0x2), S(actor, 0x6), 0, 0);
                ball_dir = B((unsigned char *)W(scratch, 0x2F4), 0x3A4);
                ang_far = ((int)((ang - ball_dir) & 0xFF) >= 0x30);
            } else {
                ang = func_8005B36C(S(actor, 0x2), S(actor, 0x6), 0, 0);
                ball_dir = B((unsigned char *)W(scratch, 0x2F4), 0x3A4);
                ang_far = ((int)((ball_dir - ang) & 0xFF) >= 0x30);
            }
            if (ang_far && H(scratch, 0xE0) == 0) {
                signed short bz = S((unsigned char *)W(scratch, 0x2F4), 0x6);
                if (bz < 0) {
                    bz = -bz;
                }
                if (bz < 0x433) {
                    unsigned int ev = W(scratch, 0x2B0);
                    if (ev == 0xB || ev == 0xD) {
                        break;
                    }
                }
            }
        }

        if (B(D_800E7DE9, 0) == 1) {
            pk_tbl = func_8001D004(0x3F);
            kick_num = B(D_8010A400, 0);
            slot_idx = (kick_num & 1U) * 5U + ((kick_num % 10U) >> 1);
            B(D_800E6D60, 0) = (unsigned char)slot_idx;
            B(pk_tbl + slot_idx * 12, 3) = 0x80;

            if ((func_80083D94(B(scratch, 0x2DB), B(scratch, 0x2DC)) & 0xFF) != 0 &&
                func_8001E6B8(3) != 0) {
                next_sub = 0x30;
            } else {
                r = func_8001E6B8(0x10);
                if (r < 0xA) {
                    next_sub = (r < 3) ? 0x50 : 0x40;
                } else {
                    next_sub = 0x50;
                }
            }
            B(actor, 0x397) = next_sub;
            func_80059420(actor, 1);
        } else {
            B(D_800E7DE9, 0) = 0;
            func_800AB620(0x51C);
            pk_tbl = func_8001D004(0x3F);
            kick_num = B(D_8010A400, 0);
            slot_idx = (kick_num & 1U) * 5U + ((kick_num % 10U) >> 1);
            B(D_800E6D60, 0) = (unsigned char)slot_idx;
            B(pk_tbl + slot_idx * 12, 3) = 0x90;

            r = func_8001E6B8(0x10);
            if (r >= 0 && r < 4) {
                func_8005BA30(actor, 0x33, 0);
                next_sub = 0x68;
            } else if (r >= 0 && r < 0xC) {
                func_8005BA30(actor, 0x37, 0);
                next_sub = 0x70;
            } else {
                func_8005BA30(actor, 0x38, 0);
                next_sub = 0x78;
            }
            B(actor, 0x397) = next_sub;
            func_80059420(actor, 0);
        }

        H(scratch, 0x294) = 0;
        B(D_800FF748, 0x65E4) = 0;
        B(D_800FF748, 0x660C) = 0;
        func_800171DC();
        break;
    }

    case 0x30: {
        unsigned char heading;
        unsigned int neg_h;
        int facing_zero;

        func_8005BD1C(actor);
        func_800479B8(actor, 0);
        H(scratch, 0xF8) = 0;
        H(scratch, 0xFC) = 0;

        heading = B(actor, 0x3AA);
        neg_h = (-heading) & 0xFF;
        if (neg_h < 0x81U) {
            facing_zero = ((int)neg_h < 0x10);
        } else {
            facing_zero = ((signed char)heading < 0x10);
        }
        if (facing_zero) {
            B(actor, 0x3B0) = 2;
            if (H(actor, 0x38E) != 0x26) {
                func_8005BA30(actor, 0x26, 0xA);
            }
            B(actor, 0x3AB) = (unsigned char)func_8005B36C(S(actor, 0x2), S(actor, 0x6), 0, 0);
            H(D_800F3566, 0) = 0x80;
            B(actor, 0x396) = 0x33;
            B(actor, 0x397) = 0x40;
        }
        break;
    }

    case 0x40: {
        unsigned int target_ang;
        unsigned int diff;
        int aligned;
        unsigned int spd;

        func_8005BD1C(actor);
        target_ang = func_8005B36C(S(actor, 0x2), S(actor, 0x6), 0, S(actor, 0x3C4));
        diff = (target_ang - B(actor, 0x3AA)) & 0xFF;
        if (diff < 0x81U) {
            aligned = ((int)diff < 0x21);
        } else {
            aligned = ((int)((B(actor, 0x3AA) - target_ang) & 0xFF) < 0x21);
        }
        if (aligned) {
            B(actor, 0x3AA) = (unsigned char)func_8005B7CC(target_ang & 0xFF, B(actor, 0x3AA), 0xC);
            B(actor, 0x3A4) = (unsigned char)target_ang;
            if (H(actor, 0x38E) != 2) {
                func_8005BA30(actor, 2, B(actor, 0x390) % 18U);
            }
            spd = W(actor, 0x3A0) + 0xC;
            W(actor, 0x3A0) = (spd < 0x31U) ? spd : 0x30;
        } else {
            unsigned char h;
            if (H(actor, 0x38E) != 5) {
                func_8005BA30(actor, 5, 0);
            }
            h = (unsigned char)func_8005B7CC(target_ang & 0xFF, B(actor, 0x3AA), 8);
            B(actor, 0x3AA) = h;
            B(actor, 0x3A4) = h;
        }
        break;
    }

    case 0x50:
        func_8005BA30(actor, 5, 0);
        B(actor, 0x3AB) = (unsigned char)func_8005B36C(S(actor, 0x2), S(actor, 0x6), 0, S(actor, 0x3C4));
        B(actor, 0x397)++;
        /* fall through */

    case 0x51: {
        unsigned char h;
        func_8005BD1C(actor);
        h = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 8);
        B(actor, 0x3AA) = h;
        B(actor, 0x3A4) = h;
        if (h == B(actor, 0x3AB)) {
            B(actor, 0x397)++;
        }
        break;
    }

    case 0x52:
        func_8005BD1C(actor);
        SW(actor, 0x8) = (int)func_8001E694(B(actor, 0x3A4), 0xC);
        SW(actor, 0x10) = (int)func_8001E5A4(B(actor, 0x3A4), 0xC);
        S(actor, 0x2) += (signed short)SW(actor, 0x8);
        S(actor, 0x6) += (signed short)SW(actor, 0x10);
        break;

    case 0x60: {
        int anim;
        switch (func_8001E6B8(8)) {
        case 0:
        case 1:
            anim = 0x38;
            break;
        case 2:
        case 3:
        case 4:
            anim = 0x37;
            break;
        default:
            anim = 0x33;
            break;
        }
        func_8005BA30(actor, anim, 0);
        B(actor, 0x397)++;
        break;
    }

    case 0x61:
        switch (H(actor, 0x38E)) {
        case 0x33:
            if (B(actor, 0x390) != 0xF) {
                func_8005BD1C(actor);
            }
            break;
        case 0x37:
            if (B(actor, 0x390) != 0x1B) {
                func_8005BD1C(actor);
            }
            break;
        case 0x38:
            if (B(actor, 0x390) != 0xD) {
                func_8005BD1C(actor);
            }
            break;
        default:
            break;
        }
        break;

    case 0x68:
        if (B(actor, 0x390) < 0xF) {
            func_8005BD1C(actor);
        }
        break;

    case 0x70:
        if (B(actor, 0x390) < 0x1B) {
            func_8005BD1C(actor);
        }
        break;

    case 0x78:
        if (B(actor, 0x390) < 0xD) {
            func_8005BD1C(actor);
        }
        break;

    case 0x80:
        if (B(actor, 0x390) < 0x2D) {
            func_8005BD1C(actor);
        }
        break;

    case 0x88:
        if (H(actor, 0x38E) != 0x5A) {
            func_8005BA30(actor, 0x5A, 0);
        }
        W(actor, 0x3A0) = 0;
        B(actor, 0x397) = 0x90;
        break;

    case 0x90:
        func_8005BD1C(actor);
        break;

    case 0x98:
        if (H(actor, 0x38E) != 1) {
            func_8005BA30(actor, 1, 0);
        }
        func_8005BD1C(actor);
        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(0, B(actor, 0x3AA), 8);
        break;

    default:
        break;
    }

    func_8005B908(actor);
}
