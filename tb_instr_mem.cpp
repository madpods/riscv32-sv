#include "Vinstr_mem.h"
#include "verilated.h"
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <cstdio>

Vinstr_mem* dut;
int errors = 0;

void check(const char* name, uint32_t got, uint32_t expected) {
    if (got != expected) {
        printf("FAIL [%s]: got=0x%08x expected=0x%08x\n", name, got, expected);
        errors++;
    } else {
        printf("PASS [%s]: 0x%08x\n", name, got);
    }
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);

    // instr_mem.sv loads "program.hex" (relative to the process's cwd) via
    // $readmemh in an initial block, so write a fixture before constructing
    // the DUT.
    {
        std::ofstream hex("program.hex");
        hex << "DEADBEEF\n"
            << "12345678\n"
            << "FFFFFFFF\n";
    }

    dut = new Vinstr_mem;
    dut->eval(); // runs the $readmemh initial block

    dut->addr = 0x00000000;
    dut->eval();
    check("word0", dut->instr, 0xDEADBEEF);

    dut->addr = 0x00000004;
    dut->eval();
    check("word1", dut->instr, 0x12345678);

    dut->addr = 0x00000008;
    dut->eval();
    check("word2", dut->instr, 0xFFFFFFFF);

    // Words beyond the hex file default to 0
    dut->addr = 0x0000000C;
    dut->eval();
    check("word3_unloaded_default", dut->instr, 0x00000000);

    // Byte-offset bits within a word are ignored (word-addressed memory)
    dut->addr = 0x00000002;
    dut->eval();
    check("byte_offset_ignored", dut->instr, 0xDEADBEEF);

    delete dut;
    std::remove("program.hex");

    if (errors == 0) {
        printf("\nAll tests PASSED\n");
        return 0;
    } else {
        printf("\n%d test(s) FAILED\n", errors);
        return 1;
    }
}
