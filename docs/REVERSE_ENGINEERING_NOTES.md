# Reverse-engineering notes (WE3 Redux)

Verified findings from `SLPM_861.62` / overlays. File offset = VRAM address - 0x80010000 + 0x800 - 0x64.

## Confirmed
| What | Where | Notes |
|---|---|---|
| Player speed table | `D_800C6AE4` (exe 0xB72E4-0x64..) = `5,5,5,5,6,7,8,9,10,16` | max speed = `0x35 + table[speed level]` (func_80023078) |
| Acceleration table | `D_800C6BF0` s16 = `-3,-3,-2,-2,-1,-1,0,0,3,6` | accel divisor = `18 - table[level]` |
| Ball physics | `func_8004BAA4` | gravity 736 raw/frame², rolling `h -= h>>6`, bounce stop `0x5BF` |
| National flag shape per team | exe file offset `0xB83D8`, 43 bytes | 28 distinct shapes, saved in `assets/database/flag_shapes.json` |
| Formation slot roles | `D_800CA30C` 11 bytes per formation id | GK/DF/MF/FW counts |
| Injury substitution priority | `D_800CA458`, used by `func_800730F8` | this is injury/bench replacement, not a red-card rebalance |

## Corrections
- An earlier note claimed the flag CLUT table was at `SELECT.BIN` 0x2A9BE. That address is **MIPS code**, not palette data. The earlier "Germany/Spain match" was a coincidence.

## Goalkeeper (ported from `func_8008F2DC`, `func_80094EEC`, `func_80094DC4`, `func_8001E6B8`)
| Rule | Source | Port |
|---|---|---|
| Keeper reacts only when ball timer (`scratch+0x31C`, frames since last touch) >= 8 | `func_80094EEC` | `Ball.flight_frames`, `GK_REACT_FRAMES` |
| Dive aims at ball position 7 frames ahead; velocity = delta/7 per frame; contact on frame 7 | `func_8008F2DC` case 0/1 | `_gk_start_dive`, `GK_DIVE_FRAMES` |
| Touch radius 128 units (0.512 m) horizontally and vertically | `func_80094EEC` | `GK_TOUCH_RADIUS` |
| Catch vs spill: spill if `timer < 29-level` and ball speed >= 0x3000; never spill if `timer >= 48`; else spill if `rand(abs(level-7)*3072) + (timer-22)*768 + 0xA000 < speed` | `func_80094DC4` | `_gk_should_spill` |
| Spill = ball pops up (lift 16 -> ~5.5 m/s), pushed 16..23 units/frame along its own heading +/- 16 units | `func_8008F2DC` case 7 | `_gk_touch_ball` |
| Hold = `func_80094FF4`: ball becomes attached to the keeper | `func_80094FF4` | `set_owner_player` |

Estimates, not recovered from code: dive reach (`GK_DIVE_BASE + 0.10*defense`), and `level = defense stat` (the C reads `record[0x3B9]/10`, whose writer was not located).
Also fixed: ball curve was far too strong; the C turns the heading 1 angle unit every `spin_interval` frames.

## Function identities (checked against the C, corrections to earlier notes)
| Function | Actually is | Earlier mis-citation |
|---|---|---|
| `func_8007CC00` | replay quarter router (roster quarter record from replay minute) | cited as foul/card logic |
| `func_8001F2D0` | match-end/result setup (scratch 0x309, 0x2E1 mode) | cited as offside |
| `func_800730F8` | injury/bench substitution using `D_800CA458` | cited as red-card rebalance |
| `func_80081B94`, `func_80084750` | per-mode dispatchers on scratch 0x1F80029B (phases 8 and 10) | not used |
| `func_80040DD4`, `func_80041898`, `func_800420E4` | tackle/contact checks, **assembly only** (0xAC4/..), no C | not used |

Match phase table (`0x1F80029A`): 0 `func_800769C0`, 1 `func_80076E7C` (open play: clock, advantage zone 0x22E3, injury time), 2 `80077270`, 3 `8007A2A8`, 4 `8007B39C`, 6 `8007BE38`, 7 `8007CEDC`, 8 `80081B94` (6 modes), 9 `80083EE8`, 10 `80084750` (5 modes), 11 `80087CCC`, 12 `80089C70`, 14 `80075BBC`.

## Open
- Real foul/tackle-contact and offside rules: only in MIPS asm (`func_80040DD4` ...), not yet decompiled.
- Flag palettes (CLUT) for the 43 teams: not found in `SELECT.BIN`, `DATSEL.BIN` (only 23 flag-like palettes in its 192), `HATA_CSR.BIN` (no CLUT descriptors), `SLPM_861.62` (no contiguous 43x16 block).
- `0x3BA` (dribble/sprint bonus) writer not located.
- `func_80054F1C` shot-decision random helper semantics.
