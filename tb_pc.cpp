#include "Vpc.h"
#include "verilated.h"
#include <cstdio>

Vpc* dut;
int timestamp = 0;
int errors = 0;

void tick() {
    dut->clk = 0; dut->eval();
    dut->clk = 1; dut->eval();
    timestamp++;
}

void check(const char* name, uint32_t got, uint32_t expected) {
    if (got != expected) {
        printf("FAIL [%s] at t=%d: got=%u expected=%u\n", name, timestamp, got, expected);
        errors++;
    } else {
        printf("PASS [%s] at t=%d: pc_current=%u\n", name, timestamp, got);
    }
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    dut = new Vpc;

    // Async reset
    dut->rst_n = 0;
    dut->pc_next = 0xDEADBEEF;
    dut->eval();
    check("async_reset", dut->pc_current, 0);

    // Reset held across a clock edge
    tick();
    check("reset_held", dut->pc_current, 0);

    // Release reset, pc_current follows pc_next on the clock edge
    dut->rst_n = 1;
    dut->pc_next = 0x00000004;
    tick();
    check("first_update", dut->pc_current, 0x00000004);

    dut->pc_next = 0x00000008;
    tick();
    check("second_update", dut->pc_current, 0x00000008);

    // Async reset mid-run, independent of clock
    dut->rst_n = 0;
    dut->eval();
    check("async_reset_midrun", dut->pc_current, 0);

    // Coming out of reset, pc_next held stable until next edge
    dut->rst_n = 1;
    dut->pc_next = 0x1000;
    tick();
    check("resume_after_reset", dut->pc_current, 0x1000);

    delete dut;

    if (errors == 0) {
        printf("\nAll tests PASSED\n");
        return 0;
    } else {
        printf("\n%d test(s) FAILED\n", errors);
        return 1;
    }
}
