#include "Vdata_mem.h"
#include "verilated.h"
#include <cstdio>
#include <cstdint>

Vdata_mem* dut;
int timestamp = 0;
int errors = 0;

void tick() {
    dut->clk = 0; dut->eval();
    dut->clk = 1; dut->eval();
    timestamp++;
}

void check(const char* name, uint32_t got, uint32_t expected) {
    if (got != expected) {
        printf("FAIL [%s] at t=%d: got=0x%08x expected=0x%08x\n", name, timestamp, got, expected);
        errors++;
    } else {
        printf("PASS [%s] at t=%d: 0x%08x\n", name, timestamp, got);
    }
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    dut = new Vdata_mem;

    // mem_read gates the output — should read 0 even with garbage/uninitialized memory
    dut->mem_read = 0;
    dut->mem_write = 0;
    dut->addr = 0;
    dut->eval();
    check("read_gated_off", dut->read_data, 0);

    // Write 0xDEADBEEF to word 0
    dut->mem_write = 1;
    dut->addr = 0x00000000;
    dut->write_data = 0xDEADBEEF;
    tick();

    // Read it back
    dut->mem_write = 0;
    dut->mem_read = 1;
    dut->eval();
    check("read_after_write_word0", dut->read_data, 0xDEADBEEF);

    // Gating still holds after a write — mem_read=0 must force 0 regardless of contents
    dut->mem_read = 0;
    dut->eval();
    check("read_gated_off_after_write", dut->read_data, 0);

    // Write a different value to word 1 (addr 4)
    dut->mem_write = 1;
    dut->mem_read = 0;
    dut->addr = 0x00000004;
    dut->write_data = 0x12345678;
    tick();

    // word 0 must be untouched
    dut->mem_write = 0;
    dut->mem_read = 1;
    dut->addr = 0x00000000;
    dut->eval();
    check("word0_unaffected_by_word1_write", dut->read_data, 0xDEADBEEF);

    // word 1 holds the new value
    dut->addr = 0x00000004;
    dut->eval();
    check("word1_value", dut->read_data, 0x12345678);

    // Write disabled — a write attempt with mem_write=0 must not modify memory
    dut->mem_write = 0;
    dut->addr = 0x00000000;
    dut->write_data = 0xFFFFFFFF;
    tick();
    dut->mem_read = 1;
    dut->addr = 0x00000000;
    dut->eval();
    check("write_disabled_no_change", dut->read_data, 0xDEADBEEF);

    delete dut;

    if (errors == 0) {
        printf("\nAll tests PASSED\n");
        return 0;
    } else {
        printf("\n%d test(s) FAILED\n", errors);
        return 1;
    }
}
