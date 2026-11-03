# CHIP-8 Studio (`3ds_chip8`)

<p align="center">
  <img src="docs/icon.png" width="72" height="72" alt="3ds_chip8 icon" />
  &nbsp;&nbsp;&nbsp;
  <img src="docs/banner.png" width="256" height="128" alt="3ds_chip8 3DS banner" />
</p>

<p align="center">
  <img src="docs/screenshot.png" width="360" alt="3ds_chip8 dual-screen 3DS preview" />
</p>

> **VM & Hex Disassembler (`CTR-P-CHP8`, UniqueId `0x0D3EA`)** — Part of the [**lafiabrew**](https://github.com/orgs/lafiabrew/repositories) Nintendo 3DS Homebrew Suite.

35-opcode CHIP-8 virtual machine, live instruction disassembler, `V0..VF` register inspector, and 16-key touch hex keypad.

---

## Architecture & Data Flow

```mermaid
flowchart LR
    ROM[Built-in or SD .ch8 ROM] --> VM[CHIP-8 CPU + 4KB RAM]
    VM --> TOP[Top Screen 64x32 Display]
    VM --> BOT[Bottom 16-Key Hex Pad + V0..VF Debugger]
```

---

## Controls & Touch Ergonomics

| Button / Input | Action |
|---|---|
| `A` | Single-step one CHIP-8 instruction |
| `X` | Toggle continuous execution vs single-step debugger |
| `L` / `R` | Cycle built-in demo ROMs |
| `Bottom Touch Keypad` | 16-key hexadecimal keypad (`0..F`) + live `V0..VF` register view |

---

## Build, Verify & Install

```bash
make test
make 3ds && make verify-3ds
make qr
```
