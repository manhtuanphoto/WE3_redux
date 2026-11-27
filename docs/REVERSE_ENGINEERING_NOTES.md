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

## Open
- Flag palettes (CLUT) for the 43 teams: not found in `SELECT.BIN`, `DATSEL.BIN` (only 23 flag-like palettes in its 192), `HATA_CSR.BIN` (no CLUT descriptors), `SLPM_861.62` (no contiguous 43x16 block).
- `0x3BA` (dribble/sprint bonus) writer not located.
- `func_80054F1C` shot-decision random helper semantics.
