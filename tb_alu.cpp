#include "Valu.h"
#include "verilated.h"
#include <cstdio>
#include <cstdint>

Valu* dut;
int errors = 0;

void check(const char* name, uint32_t got, uint32_t expected) {
    if (got != expected) {
        printf("FAIL [%s]: got=0x%08x expected=0x%08x\n", name, got, expected);
        errors++;
    } else {
        printf("PASS [%s]: 0x%08x\n", name, got);
    }
}

void run(const char* name, uint32_t a, uint32_t b, uint8_t op, uint32_t expected) {
    dut->a = a; dut->b = b; dut->alu_op = op;
    dut->eval();
    check(name, dut->result, expected);
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    dut = new Valu;

    run("add",        7, 5,  0b0000, 12);
    run("sub",         5, 7,  0b0001, (uint32_t)-2);
    run("and",         0xF0F0F0F0, 0x0FF00FF0, 0b0010, 0x00F000F0);
    run("or",          0xF0F0F0F0, 0x0FF00FF0, 0b0011, 0xFFF0FFF0);
    run("xor",         0xFF00FF00, 0x0F0F0F0F, 0b0100, 0xF00FF00F);
    run("sll",         0x00000001, 4,          0b0101, 0x00000010);
    run("srl_pos",     0x80000000, 4,          0b0110, 0x08000000);
    run("sra_neg",     0x80000000, 4,          0b0111, 0xF8000000); // sign-extends
    run("sra_pos",     0x40000000, 4,          0b0111, 0x04000000); // no sign bit set
    run("slt_true",    (uint32_t)-1, 1,        0b1000, 1);          // -1 < 1 signed
    run("slt_false",   1, (uint32_t)-1,        0b1000, 0);          // 1 < -1 signed -> false
    run("sltu_true",   1, (uint32_t)-1,        0b1001, 1);          // 1 < 0xFFFFFFFF unsigned
    run("sltu_false",  (uint32_t)-1, 1,        0b1001, 0);          // 0xFFFFFFFF < 1 unsigned -> false
    run("default_op",  1, 1,                   0b1111, 0);          // unused opcode -> 0

    // zero flag
    dut->a = 5; dut->b = 5; dut->alu_op = 0b0001; // sub -> 0
    dut->eval();
    check("zero_flag_set", dut->zero, 1);

    dut->a = 5; dut->b = 6; dut->alu_op = 0b0001; // sub -> nonzero
    dut->eval();
    check("zero_flag_clear", dut->zero, 0);

    delete dut;

    if (errors == 0) {
        printf("\nAll tests PASSED\n");
        return 0;
    } else {
        printf("\n%d test(s) FAILED\n", errors);
        return 1;
    }
}
