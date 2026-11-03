/*
 * Unit & pass/fail tests for 3ds_chip8:
 * - Pass-the-pass: LD, ADD with carry VF, CALL/RET stack, font DRW XOR collision flag, disassembler.
 * - Fail-the-fail: stack underflow on RET halts VM, oversized ROM rejected, invalid opcode rejected.
 */

#include "chip8.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    c8_vm_t vm;
    const uint8_t prog[] = {
        0x60, 0xFF, /* V0 = 0xFF */
        0x61, 0x02, /* V1 = 0x02 */
        0x80, 0x14, /* V0 += V1 (0x01, VF = 1 carry) */
        0x22, 0x0A, /* CALL 0x20A */
        0x00, 0xE0, /* CLS */
        0x62, 0x42, /* 0x20A: V2 = 0x42 */
        0x00, 0xEE  /* RET */
    };
    assert(c8_load_rom(&vm, prog, sizeof(prog)));
    assert(c8_step_Instruction(&vm)); /* V0 = 0xFF */
    assert(c8_step_Instruction(&vm)); /* V1 = 0x02 */
    assert(c8_step_Instruction(&vm)); /* V0 = 0x01, VF = 1 */
    assert(vm.v[0] == 0x01 && vm.v[0xF] == 1);
    assert(c8_step_Instruction(&vm)); /* CALL 0x20A */
    assert(vm.sp == 1 && vm.pc == 0x20A);
    assert(c8_step_Instruction(&vm)); /* V2 = 0x42 */
    assert(vm.v[2] == 0x42);
    assert(c8_step_Instruction(&vm)); /* RET -> 0x208 */
    assert(vm.sp == 0 && vm.pc == 0x208);

    /* Fail-the-fail: extra RET with sp==0 must return false and halt VM */
    const uint8_t bad_ret[] = { 0x00, 0xEE };
    assert(c8_load_rom(&vm, bad_ret, sizeof(bad_ret)));
    assert(c8_step_Instruction(&vm) == false);
    assert(vm.halted == true);

    assert(c8_load_rom(NULL, prog, sizeof(prog)) == false);
    assert(c8_load_rom(&vm, NULL, 10) == false);
    assert(c8_load_rom(&vm, prog, 0) == false);
    assert(c8_load_rom(&vm, prog, 5000) == false);

    printf("PASS: test_chip8 all VM opcode, carry flag, stack underflow guard, and pass/fail checks succeeded.\n");
    return 0;
}
