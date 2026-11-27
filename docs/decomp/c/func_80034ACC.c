/*
 * func_80034ACC
 *
 * Purpose:
 *   Executes a kick/pass/clearance for an actor toward either a targeted
 *   teammate (+0x3C2) or preset target coordinates (+0x3C8/+0x3CA). Clamps the
 *   target X into [0x27E2..0x3220] in the attacking direction, computes kick
 *   heading with stat-based jitter, picks kick power and loft by pass type
 *   (+0x3C4: 0=ground, 1=through, 2=lofted), plays kick SFX (func_800AB620),
 *   kicks the ball via func_8004B0F0, sets ball spin/curve (+0x3C8/+0x3CA),
 *   syncs ball position from scratchpad, and updates ball/actor state.
 *
 * Arguments:
 *   actor (unsigned char *): kicking player's 1000-byte actor struct
 *
 * Return value:
 *   void
 *
 * Scratchpad / Actor fields read or written:
 *   Scratch reads:  0x36 (s16 kick origin x), 0x4E (s16 kick origin y),
 *                   0x66 (s16 kick origin z), 0x29A (match phase),
 *                   0x2F4 (ptr to ball actor)
 *   Scratch writes: 0x2B0 = 3 (ball event code), 0x31C = attack_dir ^ 1
 *   Actor reads:    +0x02 (x), +0x38E (anim id), +0x392 (team slot),
 *                   +0x398 (side), +0x399 (attack dir), +0x39A (target flag),
 *                   +0x3AA (body heading), +0x3C2 (target id), +0x3C4 (pass type),
 *                   +0x3C8 (target x), +0x3CA (target z)
 *   Actor writes:   +0x3C4 (pass type), +0x3D1 = 3
 *   Ball writes:    +0x02/+0x04/+0x06 (xyz from scratch 0x36/0x4E/0x66),
 *                   +0x396 = 4, +0x3B2 = 2, +0x3C8 (spin/loft), +0x3CA (curve dir)
 *
 * Magic numbers / units (250 raw = 1 m, 30 frames/s):
 *   0x180..0x600  = 384..1536 raw (1.536..6.144 m) open-play target lead
 *   0x100..0x1FF  = 256..511 raw (1.024..2.044 m) fallback lead
 *   0x27E2..0x3220= 10210..12832 raw (40.84..51.328 m) target X clamp range
 *   0xC00..0x3000 = 3072..12288 raw (12.288..49.152 m) scaled kick distance
 *   0xC0          = 192 max kick power; 0x507/0x508 = kick sound IDs
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010C32E[];

unsigned int func_8001E6B8(unsigned int n);
unsigned int func_8005B61C(int x1, int z1, int x2, int z2);
int func_8005C7A0(int n);
unsigned int func_800B1128(unsigned int val);
unsigned int func_8002EB3C(unsigned int dist, unsigned int rand_param);
void func_800AB620(unsigned int sfx_id);
unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);
void func_8004B0F0(unsigned char *actor, unsigned int heading, unsigned int power, unsigned int lift);
void func_80089D00(unsigned char *actor, int mode, int tx, int tz);
void func_80035320(unsigned char *actor, int tx, int tz);
void func_8004B278(void);
void func_80056D10(unsigned char *actor);

void func_80034ACC(unsigned char *actor)
{
    int dir_sign;
    signed short target_idx;
    unsigned short tx;
    unsigned short tz;
    int signed_tx;
    unsigned int heading;
    unsigned int stat;
    unsigned int power = 0;
    unsigned int lift = 0;
    unsigned char *ball;
    unsigned int r;
    unsigned int ang_diff;
    int ang_term;
    int tx_s, tz_s;

    dir_sign = (B(actor, 0x399) != 0) ? -1 : 1;
    target_idx = S(actor, 0x3C2);

    if (target_idx != 0) {
        unsigned char *target = D_800FF748 + (target_idx % 24) * 0x3E8;
        if (B(SCRATCH, 0x29A) == 1) {
            signed short ax = S(actor, 2);
            signed short rx = S(target, 2);
            int lead;
            if (dir_sign * rx < dir_sign * ax) {
                lead = ax - rx;
                if (lead < 0) {
                    lead = -lead;
                }
                lead >>= 1;
                if ((signed short)lead >= 0x600) {
                    lead = 0x600;
                } else if ((signed short)lead < 0x181) {
                    lead = 0x180;
                }
                lead = (signed short)lead;
            } else {
                lead = (int)func_8001E6B8(0x100) + 0x100;
            }
            tx = (unsigned short)(H(target, 2) + dir_sign * lead);
        } else {
            tx = H(target, 2);
        }
        tz = H(target, 6);
    } else {
        tx = H(actor, 0x3C8);
        tz = H(actor, 0x3CA);
    }

    signed_tx = dir_sign * (signed short)tx;
    if (signed_tx >= 0x3221) {
        tx = (unsigned short)(dir_sign * 0x3220);
    } else if (signed_tx < 0x27E2) {
        tx = (unsigned short)(dir_sign * 0x27E2);
    }

    heading = func_8005B61C(S(SCRATCH, 0x36), S(SCRATCH, 0x66), (signed short)tx, (signed short)tz);
    stat = H(D_8010C32E, B(actor, 0x392) * 12 + B(actor, 0x398) * 264) & 0xF;
    heading += (unsigned int)func_8005C7A0((signed short)(10 - (int)stat));

    if (B(actor, 0x39A) != 0) {
        switch (func_8001E6B8(8)) {
        case 0:
        case 1:
        case 2:
        case 3:
            S(actor, 0x3C4) = 0;
            break;
        case 4:
        case 5:
        case 6:
            S(actor, 0x3C4) = 1;
            break;
        case 7:
        default:
            S(actor, 0x3C4) = 2;
            break;
        }
    }

    switch (S(actor, 0x3C4)) {
    case 0: {
        int dx, dz;
        unsigned int dist, scaled, denom;
        int jitter;

        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        dx = S(ball, 2) - (signed short)tx;
        dz = S(ball, 6) - (signed short)tz;
        dist = func_800B1128((unsigned int)(dx * dx + dz * dz));
        scaled = (dist * 12) / 10;
        if (scaled < 0xC00) {
            scaled = 0xC00;
        }
        if (scaled >= 0x3001) {
            scaled = 0x3000;
        }
        jitter = (signed short)func_8005C7A0(0x10);
        denom = (scaled / 72) + (unsigned int)(jitter + 40);
        power = denom;
        lift = (scaled / denom) >> 1;
        if ((int)lift < 0x18) {
            lift = 0x18;
        }
        func_800AB620(0x507);
        break;
    }

    case 1: {
        int dx, dz;
        unsigned int dist, scaled, denom;
        int jitter;

        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        dx = S(ball, 2) - (signed short)tx;
        dz = S(ball, 6) - (signed short)tz;
        dist = func_800B1128((unsigned int)(dx * dx + dz * dz));
        scaled = (dist * 11) / 10;
        if (scaled < 0xC00) {
            scaled = 0xC00;
        }
        if (scaled >= 0x3001) {
            scaled = 0x3000;
        }
        jitter = (signed short)func_8005C7A0(0x18);
        denom = (scaled / 60) + (unsigned int)(jitter + 48);
        power = denom;
        lift = (scaled / denom) >> 1;
        if ((int)lift < 0x10) {
            lift = 0x10;
        }
        func_800AB620(0x507);
        break;
    }

    default: {
        int dx, dz;
        unsigned int dist, scaled;

        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        dx = S(ball, 2) - (signed short)tx;
        dz = S(ball, 6) - (signed short)tz;
        dist = func_800B1128((unsigned int)(dx * dx + dz * dz));
        scaled = (dist * 17) / 10;
        if (scaled < 0xC00) {
            scaled = 0xC00;
        }
        power = func_8002EB3C(scaled, func_8001E6B8(0x800) + 0x2800) & 0xFF;
        lift = scaled >> 9;
        if ((int)lift >= 0xB) {
            lift = 0xA;
        }
        func_800AB620(0x508);
        S(actor, 0x3C4) = 2;
        break;
    }
    }

    if (H(actor, 0x38E) == 0x14) {
        heading = func_8005B7CC(heading & 0xFF, B(actor, 0x3AA), 0x42);
    }
    if (power >= 0xC0) {
        power = 0xC0;
    }

    func_8004B0F0(actor, heading & 0xFF, power, lift);

    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    H(ball, 0x3CA) = (((heading - B(actor, 0x3AA)) & 0xFF) < 0x81) ? 1 : 0;

    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    r = func_8001E6B8(2);
    ang_diff = (B(actor, 0x3AA) - heading) & 0xFF;
    if (ang_diff < 0x81) {
        ang_term = (int)ang_diff >> 6;
    } else {
        ang_term = (int)((heading - B(actor, 0x3AA)) & 0xFF) >> 6;
    }
    H(ball, 0x3C8) = (unsigned short)((12 - (int)r) - ang_term);

    tx_s = (signed short)tx;
    tz_s = (signed short)tz;
    func_80089D00(actor, 0, tx_s, tz_s);

    W(SCRATCH, 0x2B0) = 3;
    H(SCRATCH, 0x31C) = (unsigned short)(B(actor, 0x399) ^ 1);
    func_80035320(actor, tx_s, tz_s);

    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    H(ball, 2) = H(SCRATCH, 0x36);
    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    H(ball, 4) = H(SCRATCH, 0x4E);
    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    H(ball, 6) = H(SCRATCH, 0x66);
    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    B(ball, 0x3B2) = 2;
    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    B(ball, 0x396) = 4;

    if (S(actor, 0x3C4) < 2) {
        func_8004B278();
    }
    B(actor, 0x3D1) = 3;
    func_80056D10(actor);
}
