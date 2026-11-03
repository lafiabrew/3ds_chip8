#ifndef CHIP8_H
#define CHIP8_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define C8_MEM_SIZE  4096
#define C8_PROG_BASE 0x200
#define C8_DISP_W    64
#define C8_DISP_H    32

typedef struct {
    uint8_t memory[C8_MEM_SIZE];
    uint8_t v[16];
    uint16_t i_reg;
    uint16_t pc;
    uint16_t stack[16];
    uint8_t sp;
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint8_t display[C8_DISP_H * C8_DISP_W];
    uint16_t keys_held; /* bitmask for hex keypad 0x0..0xF */
    uint32_t rng_state;
    uint32_t cycles;
    uint16_t last_opcode;
    bool halted;
} c8_vm_t;

void c8_init(c8_vm_t *vm);
bool c8_load_rom(c8_vm_t *vm, const uint8_t *rom, size_t len);
bool c8_load_builtin_demo(c8_vm_t *vm, int demo_id, char *out_name, size_t cap);
bool c8_step_Instruction(c8_vm_t *vm);
void c8_disassemble(uint16_t opcode, char *out_str, size_t cap);

#endif /* CHIP8_H */
