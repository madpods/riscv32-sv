#include "Vaccumulator.h"
#include "verilated.h"
#include <cstdio>
#include <cstdlib>

Vaccumulator* dut;
int timestamp = 0;
int errors = 0;

void tick() {
    dut->clk = 0; dut->eval();
    dut->clk = 1; dut->eval();
    timestamp++;
}

void check(const char* name, uint8_t got, uint8_t expected) {
    if (got != expected) {
        printf("FAIL [%s] at t=%d: got=%d expected=%d\n", name, timestamp, got, expected);
        errors++;
    } else {
        printf("PASS [%s] at t=%d: count_out=%d\n", name, timestamp, got);
    }
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    dut = new Vaccumulator;

    // Async reset
    dut->rst_n = 0;
    dut->load = 0;
    dut->enable = 0;
    dut->load_value = 0;
    tick(); tick();
    check("reset", dut->count_out, 0);

    dut->rst_n = 1;

    // Load a value
    dut->load = 1;
    dut->load_value = 0x2A;
    tick();
    check("load", dut->count_out, 0x2A);

    // Hold: neither load nor enable
    dut->load = 0;
    dut->enable = 0;
    tick();
    check("hold", dut->count_out, 0x2A);

    // Enable: increment
    dut->enable = 1;
    tick();
    check("enable+1", dut->count_out, 0x2B);
    tick();
    check("enable+2", dut->count_out, 0x2C);

    // Priority: load beats enable when both asserted
    dut->load = 1;
    dut->enable = 1;
    dut->load_value = 0x10;
    tick();
    check("load_priority", dut->count_out, 0x10);

    // After priority test, enable still 1, load deasserted -> should increment from loaded value
    dut->load = 0;
    tick();
    check("resume_enable", dut->count_out, 0x11);

    // Reset again mid-run (async)
    dut->rst_n = 0;
    dut->eval();
    check("async_reset", dut->count_out, 0);

    dut->rst_n = 1;
    dut->enable = 0;
    dut->load = 0;

    // Wraparound: load 0xFF then enable to wrap to 0x00
    dut->load = 1;
    dut->load_value = 0xFF;
    tick();
    check("load_ff", dut->count_out, 0xFF);
    dut->load = 0;
    dut->enable = 1;
    tick();
    check("wrap_to_00", dut->count_out, 0x00);

    delete dut;

    if (errors == 0) {
        printf("\nAll tests PASSED\n");
        return 0;
    } else {
        printf("\n%d test(s) FAILED\n", errors);
        return 1;
    }
}
