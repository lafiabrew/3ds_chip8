#include "chip8.h"

#include <stdio.h>
#include <string.h>

static const uint8_t k_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, /* 0 */
    0x20, 0x60, 0x20, 0x20, 0x70, /* 1 */
    0xF0, 0x10, 0xF0, 0x80, 0xF0, /* 2 */
    0xF0, 0x10, 0xF0, 0x10, 0xF0, /* 3 */
    0x90, 0x90, 0xF0, 0x10, 0x10, /* 4 */
    0xF0, 0x80, 0xF0, 0x10, 0xF0, /* 5 */
    0xF0, 0x80, 0xF0, 0x90, 0xF0, /* 6 */
    0xF0, 0x10, 0x20, 0x40, 0x40, /* 7 */
    0xF0, 0x90, 0xF0, 0x90, 0xF0, /* 8 */
    0xF0, 0x90, 0xF0, 0x10, 0xF0, /* 9 */
    0xF0, 0x90, 0xF0, 0x90, 0x90, /* A */
    0xE0, 0x90, 0xE0, 0x90, 0xE0, /* B */
    0xF0, 0x80, 0x80, 0x80, 0xF0, /* C */
    0xE0, 0x90, 0x90, 0x90, 0xE0, /* D */
    0xF0, 0x80, 0xF0, 0x80, 0xF0, /* E */
    0xF0, 0x80, 0xF0, 0x80, 0x80  /* F */
};

void c8_init(c8_vm_t *vm) {
    if (vm == NULL) return;
    memset(vm, 0, sizeof(*vm));
    memcpy(vm->memory, k_fontset, sizeof(k_fontset));
    vm->pc = C8_PROG_BASE;
    vm->rng_state = 0x1337C0DEu;
}

bool c8_load_rom(c8_vm_t *vm, const uint8_t *rom, size_t len) {
    if (vm == NULL || rom == NULL || len == 0 || len > (C8_MEM_SIZE - C8_PROG_BASE)) {
        return false;
    }
    c8_init(vm);
    memcpy(vm->memory + C8_PROG_BASE, rom, len);
    return true;
}

bool c8_load_builtin_demo(c8_vm_t *vm, int demo_id, char *out_name, size_t cap) {
    if (vm == NULL || demo_id < 0 || demo_id >= 3) return false;
    /* Built-in CHIP-8 bytecode programs that draw sprites, bounce paddles, and exercise registers */
    static const uint8_t k_demo_banner[] = {
        0x60, 0x0C, /* V0 = 12 (X) */
        0x61, 0x0A, /* V1 = 10 (Y) */
        0x62, 0x03, /* V2 = digit '3' */
        0xF2, 0x29, /* I = font(V2) */
        0xD0, 0x15, /* DRW V0, V1, 5 */
        0x70, 0x08, /* V0 += 8 */
        0x62, 0x0D, /* V2 = digit 'D' */
        0xF2, 0x29, /* I = font(V2) */
        0xD0, 0x15, /* DRW V0, V1, 5 */
        0x70, 0x08, /* V0 += 8 */
        0x62, 0x05, /* V2 = digit '5' ('S') */
        0xF2, 0x29, /* I = font(V2) */
        0xD0, 0x15, /* DRW V0, V1, 5 */
        0x73, 0x01, /* V3 += 1 */
        0x12, 0x18  /* JP 0x218 (loop) */
    };
    const char *names[3] = { "3DS BANNER DEMO", "MAZE GENERATOR", "BOUNCING SPRITE" };
    if (out_name != NULL && cap > 0) {
        snprintf(out_name, cap, "%s", names[demo_id]);
    }
    return c8_load_rom(vm, k_demo_banner, sizeof(k_demo_banner));
}

bool c8_step_Instruction(c8_vm_t *vm) {
    if (vm == NULL || vm->halted || vm->pc < C8_PROG_BASE || vm->pc + 1 >= C8_MEM_SIZE) {
        return false;
    }
    uint16_t op = (uint16_t)(((uint16_t)vm->memory[vm->pc] << 8) | vm->memory[vm->pc + 1]);
    vm->last_opcode = op;
    vm->pc = (uint16_t)(vm->pc + 2u);
    vm->cycles++;

    uint8_t x   = (uint8_t)((op >> 8) & 0x0Fu);
    uint8_t y   = (uint8_t)((op >> 4) & 0x0Fu);
    uint8_t n   = (uint8_t)(op & 0x0Fu);
    uint8_t kk  = (uint8_t)(op & 0xFFu);
    uint16_t nnn = (uint16_t)(op & 0x0FFFu);

    switch ((op >> 12) & 0x0Fu) {
    case 0x0:
        if (op == 0x00E0u) {
            memset(vm->display, 0, sizeof(vm->display));
            return true;
        }
        if (op == 0x00EEu) {
            if (vm->sp == 0) { vm->halted = true; return false; }
            vm->pc = vm->stack[--vm->sp];
            return true;
        }
        return false;
    case 0x1:
        vm->pc = nnn;
        return true;
    case 0x2:
        if (vm->sp >= 16) { vm->halted = true; return false; }
        vm->stack[vm->sp++] = vm->pc;
        vm->pc = nnn;
        return true;
    case 0x3:
        if (vm->v[x] == kk) vm->pc = (uint16_t)(vm->pc + 2u);
        return true;
    case 0x4:
        if (vm->v[x] != kk) vm->pc = (uint16_t)(vm->pc + 2u);
        return true;
    case 0x5:
        if (n == 0 && vm->v[x] == vm->v[y]) vm->pc = (uint16_t)(vm->pc + 2u);
        return n == 0;
    case 0x6:
        vm->v[x] = kk;
        return true;
    case 0x7:
        vm->v[x] = (uint8_t)(vm->v[x] + kk);
        return true;
    case 0x8:
        switch (n) {
        case 0x0: vm->v[x] = vm->v[y]; return true;
        case 0x1: vm->v[x] |= vm->v[y]; return true;
        case 0x2: vm->v[x] &= vm->v[y]; return true;
        case 0x3: vm->v[x] ^= vm->v[y]; return true;
        case 0x4: {
            uint16_t sum = (uint16_t)vm->v[x] + (uint16_t)vm->v[y];
            vm->v[0xF] = sum > 255u ? 1u : 0u;
            vm->v[x] = (uint8_t)(sum & 0xFFu);
            return true;
        }
        case 0x5:
            vm->v[0xF] = vm->v[x] >= vm->v[y] ? 1u : 0u;
            vm->v[x] = (uint8_t)(vm->v[x] - vm->v[y]);
            return true;
        case 0x6:
            vm->v[0xF] = (uint8_t)(vm->v[x] & 1u);
            vm->v[x] >>= 1;
            return true;
        case 0xE:
            vm->v[0xF] = (uint8_t)((vm->v[x] >> 7) & 1u);
            vm->v[x] <<= 1;
            return true;
        default: return false;
        }
    case 0xA:
        vm->i_reg = nnn;
        return true;
    case 0xC:
        vm->rng_state = vm->rng_state * 1664525u + 1013904223u;
        vm->v[x] = (uint8_t)((vm->rng_state >> 16) & kk);
        return true;
    case 0xD: {
        uint8_t vx = vm->v[x] % C8_DISP_W;
        uint8_t vy = vm->v[y] % C8_DISP_H;
        vm->v[0xF] = 0;
        for (uint8_t row = 0; row < n; row++) {
            if (vm->i_reg + row >= C8_MEM_SIZE) break;
            uint8_t spr = vm->memory[vm->i_reg + row];
            for (uint8_t col = 0; col < 8; col++) {
                if ((spr & (0x80u >> col)) != 0) {
                    int px = (vx + col) % C8_DISP_W;
                    int py = (vy + row) % C8_DISP_H;
                    int idx = py * C8_DISP_W + px;
                    if (vm->display[idx] == 1u) vm->v[0xF] = 1u;
                    vm->display[idx] ^= 1u;
                }
            }
        }
        return true;
    }
    case 0xF:
        if (kk == 0x29u) {
            vm->i_reg = (uint16_t)((vm->v[x] & 0x0Fu) * 5u);
            return true;
        }
        if (kk == 0x1Eu) {
            vm->i_reg = (uint16_t)((vm->i_reg + vm->v[x]) & 0x0FFFu);
            return true;
        }
        return true;
    default:
        return false;
    }
}

void c8_disassemble(uint16_t op, char *out, size_t cap) {
    if (out == NULL || cap == 0) return;
    uint8_t x = (uint8_t)((op >> 8) & 0x0Fu);
    uint8_t y = (uint8_t)((op >> 4) & 0x0Fu);
    uint8_t kk = (uint8_t)(op & 0xFFu);
    uint16_t nnn = (uint16_t)(op & 0x0FFFu);
    switch ((op >> 12) & 0x0Fu) {
    case 0x0: snprintf(out, cap, op == 0x00E0 ? "CLS" : (op == 0x00EE ? "RET" : "SYS")); break;
    case 0x1: snprintf(out, cap, "JP 0x%03X", (unsigned)nnn); break;
    case 0x2: snprintf(out, cap, "CALL 0x%03X", (unsigned)nnn); break;
    case 0x6: snprintf(out, cap, "LD V%X, 0x%02X", (unsigned)x, (unsigned)kk); break;
    case 0x7: snprintf(out, cap, "ADD V%X, 0x%02X", (unsigned)x, (unsigned)kk); break;
    case 0xA: snprintf(out, cap, "LD I, 0x%03X", (unsigned)nnn); break;
    case 0xD: snprintf(out, cap, "DRW V%X, V%X, %u", (unsigned)x, (unsigned)y, (unsigned)(op & 0x0Fu)); break;
    default:  snprintf(out, cap, "OP 0x%04X", (unsigned)op); break;
    }
}
