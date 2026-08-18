#include "Vregfile.h"
#include "verilated.h"
#include <cstdio>

Vregfile* dut;
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
        printf("PASS [%s] at t=%d: got=%u\n", name, timestamp, got);
    }
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    dut = new Vregfile;

    // Async reset clears all regs
    dut->rst_n = 0;
    dut->we = 0;
    dut->rs1_addr = 0;
    dut->rs2_addr = 0;
    dut->rd_addr = 0;
    dut->rd_data = 0;
    dut->eval();
    check("reset_x0", dut->rs1_data, 0);

    dut->rst_n = 1;

    // x0 always reads zero, even after an attempted write
    dut->we = 1;
    dut->rd_addr = 0;
    dut->rd_data = 0xFFFFFFFF;
    tick();
    dut->rs1_addr = 0;
    dut->eval();
    check("x0_write_ignored", dut->rs1_data, 0);

    // Write to x5, read back combinationally after the clock edge
    dut->rd_addr = 5;
    dut->rd_data = 0x12345678;
    tick();
    dut->rs1_addr = 5;
    dut->eval();
    check("write_x5", dut->rs1_data, 0x12345678);

    // Simultaneous read of rs1/rs2 from different registers
    dut->we = 1;
    dut->rd_addr = 10;
    dut->rd_data = 0xAABBCCDD;
    tick();
    dut->we = 0;
    dut->rs1_addr = 5;
    dut->rs2_addr = 10;
    dut->eval();
    check("dual_read_rs1", dut->rs1_data, 0x12345678);
    check("dual_read_rs2", dut->rs2_data, 0xAABBCCDD);

    // No write when we is deasserted
    dut->we = 0;
    dut->rd_addr = 5;
    dut->rd_data = 0xDEADDEAD;
    tick();
    dut->rs1_addr = 5;
    dut->eval();
    check("write_disabled", dut->rs1_data, 0x12345678);

    // Async reset clears everything again
    dut->rst_n = 0;
    dut->eval();
    dut->rs1_addr = 5;
    dut->rs2_addr = 10;
    dut->eval();
    check("reset_clears_x5", dut->rs1_data, 0);
    check("reset_clears_x10", dut->rs2_data, 0);

    delete dut;

    if (errors == 0) {
        printf("\nAll tests PASSED\n");
        return 0;
    } else {
        printf("\n%d test(s) FAILED\n", errors);
        return 1;
    }
}
