// Integration test for riscv_single_cycle.sv: assembles a small RV32I
// program, loads it via program.hex (read by instr_mem's $readmemh), runs
// the core for a fixed number of cycles, and checks architectural state
// (register file, data memory, PC) against hand-traced expected values.
//
// Register file, data memory, and PC are read through Verilator's `public`
// pragma accessors (see the `/* verilator public */` markers in
// riscv_single_cycle.sv, regfile.sv, data_mem.sv) since the top module
// exposes no other observable ports.

#include "Vriscv_single_cycle.h"
#include "Vriscv_single_cycle___024root.h"
#include "Vriscv_single_cycle_riscv_single_cycle.h"
#include "Vriscv_single_cycle_regfile.h"
#include "Vriscv_single_cycle_data_mem.h"
#include "verilated.h"
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <vector>

Vriscv_single_cycle* dut;
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

uint32_t reg(int i)   { return dut->rootp->riscv_single_cycle->rf->regs[i]; }
uint32_t dmem(int w)  { return dut->rootp->riscv_single_cycle->dmem->mem[w]; }
uint32_t pc()         { return dut->rootp->riscv_single_cycle->pc_current; }

// --- tiny RV32I assembler, mirrors the field layouts in imm_gen.sv/control_unit.sv ---

uint32_t r_type(uint8_t funct7, uint8_t rs2, uint8_t rs1, uint8_t funct3, uint8_t rd, uint8_t op) {
    return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | op;
}
uint32_t i_type(int32_t imm12, uint8_t rs1, uint8_t funct3, uint8_t rd, uint8_t op) {
    return (((uint32_t)imm12 & 0xFFF) << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | op;
}
uint32_t s_type(int32_t imm12, uint8_t rs2, uint8_t rs1, uint8_t funct3, uint8_t op) {
    uint32_t imm = (uint32_t)imm12 & 0xFFF;
    return ((imm >> 5) << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | ((imm & 0x1F) << 7) | op;
}
uint32_t b_type(int32_t imm13, uint8_t rs2, uint8_t rs1, uint8_t funct3, uint8_t op) {
    uint32_t imm = (uint32_t)imm13 & 0x1FFF;
    return (((imm >> 12) & 1) << 31) | (((imm >> 5) & 0x3F) << 25) | (rs2 << 20) | (rs1 << 15) |
           (funct3 << 12) | (((imm >> 1) & 0xF) << 8) | (((imm >> 11) & 1) << 7) | op;
}
uint32_t u_type(uint32_t imm20, uint8_t rd, uint8_t op) {
    return ((imm20 & 0xFFFFF) << 12) | (rd << 7) | op;
}
uint32_t j_type(int32_t imm21, uint8_t rd, uint8_t op) {
    uint32_t imm = (uint32_t)imm21 & 0x1FFFFF;
    return (((imm >> 20) & 1) << 31) | (((imm >> 1) & 0x3FF) << 21) | (((imm >> 11) & 1) << 20) |
           (((imm >> 12) & 0xFF) << 12) | (rd << 7) | op;
}

const uint8_t OP_R      = 0b0110011;
const uint8_t OP_I_ALU  = 0b0010011;
const uint8_t OP_LOAD   = 0b0000011;
const uint8_t OP_STORE  = 0b0100011;
const uint8_t OP_BRANCH = 0b1100011;
const uint8_t OP_LUI    = 0b0110111;
const uint8_t OP_JAL    = 0b1101111;

uint32_t ADDI(int rd, int rs1, int32_t imm) { return i_type(imm, rs1, 0b000, rd, OP_I_ALU); }
uint32_t ADD(int rd, int rs1, int rs2)  { return r_type(0b0000000, rs2, rs1, 0b000, rd, OP_R); }
uint32_t SUB(int rd, int rs1, int rs2)  { return r_type(0b0100000, rs2, rs1, 0b000, rd, OP_R); }
uint32_t AND(int rd, int rs1, int rs2)  { return r_type(0b0000000, rs2, rs1, 0b111, rd, OP_R); }
uint32_t OR(int rd, int rs1, int rs2)   { return r_type(0b0000000, rs2, rs1, 0b110, rd, OP_R); }
uint32_t XOR(int rd, int rs1, int rs2)  { return r_type(0b0000000, rs2, rs1, 0b100, rd, OP_R); }
uint32_t SLT(int rd, int rs1, int rs2)  { return r_type(0b0000000, rs2, rs1, 0b010, rd, OP_R); }
uint32_t SLTU(int rd, int rs1, int rs2) { return r_type(0b0000000, rs2, rs1, 0b011, rd, OP_R); }
uint32_t SW(int rs2, int32_t imm, int rs1)  { return s_type(imm, rs2, rs1, 0b010, OP_STORE); }
uint32_t LW(int rd, int32_t imm, int rs1)   { return i_type(imm, rs1, 0b010, rd, OP_LOAD); }
uint32_t BEQ(int rs1, int rs2, int32_t imm) { return b_type(imm, rs2, rs1, 0b000, OP_BRANCH); }
uint32_t BNE(int rs1, int rs2, int32_t imm) { return b_type(imm, rs2, rs1, 0b001, OP_BRANCH); }
uint32_t LUI(int rd, uint32_t imm20)        { return u_type(imm20, rd, OP_LUI); }
uint32_t JAL(int rd, int32_t imm)           { return j_type(imm, rd, OP_JAL); }

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);

    // Program: exercises arithmetic, memory, both branch outcomes, jal
    // (with link-address + skip verification), and lui. Poison values
    // (666, 888, 555) mark instructions that must be skipped by correct
    // control flow — if a branch/jump is mis-decoded, they land in the
    // register file and the corresponding check fails.
    std::vector<uint32_t> program = {
        /* 0  */ ADDI(1, 0, 5),          // x1 = 5
        /* 4  */ ADDI(2, 0, 3),          // x2 = 3
        /* 8  */ ADD(3, 1, 2),           // x3 = x1 + x2 = 8
        /* 12 */ SUB(4, 1, 2),           // x4 = x1 - x2 = 2
        /* 16 */ AND(5, 1, 2),           // x5 = x1 & x2 = 1
        /* 20 */ OR(6, 1, 2),            // x6 = x1 | x2 = 7
        /* 24 */ XOR(7, 1, 2),           // x7 = x1 ^ x2 = 6
        /* 28 */ SLT(8, 2, 1),           // x8 = (x2 < x1) = 1
        /* 32 */ SW(3, 0, 0),            // mem[0] = x3 = 8
        /* 36 */ LW(9, 0, 0),            // x9 = mem[0] = 8
        /* 40 */ BEQ(1, 1, 8),           // taken -> pc 48 (skips addr 44)
        /* 44 */ ADDI(10, 0, 666),       // poison: must be skipped
        /* 48 */ ADDI(10, 0, 42),        // x10 = 42
        /* 52 */ BEQ(1, 2, 8),           // not taken (5 != 3) -> falls through
        /* 56 */ ADDI(11, 0, 77),        // x11 = 77
        /* 60 */ JAL(12, 8),             // x12 = 64 (link), jump -> pc 68 (skips addr 64)
        /* 64 */ ADDI(13, 0, 555),       // poison: must be skipped
        /* 68 */ ADDI(14, 0, 123),       // x14 = 123
        /* 72 */ LUI(15, 0x12345),       // x15 = 0x12345000
        /* 76 */ SLTU(16, 2, 1),         // x16 = (x2 <u x1) = 1
        /* 80 */ BNE(1, 2, 8),           // taken (5 != 3) -> pc 88 (skips addr 84)
        /* 84 */ ADDI(17, 0, 888),       // poison: must be skipped
        /* 88 */ ADDI(17, 0, 17),        // x17 = 17
    };

    {
        std::ofstream hex("program.hex");
        for (uint32_t w : program) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%08x", w);
            hex << buf << "\n";
        }
    }

    dut = new Vriscv_single_cycle;

    // Async reset
    dut->rst_n = 0;
    dut->clk = 0; dut->eval();
    tick();
    tick();
    check("reset_pc", pc(), 0);
    check("reset_x1", reg(1), 0);

    dut->rst_n = 1;

    for (int i = 0; i < 20; i++) tick();

    check("x1_addi",        reg(1),  5);
    check("x2_addi",        reg(2),  3);
    check("x3_add",         reg(3),  8);
    check("x4_sub",         reg(4),  2);
    check("x5_and",         reg(5),  1);
    check("x6_or",          reg(6),  7);
    check("x7_xor",         reg(7),  6);
    check("x8_slt",         reg(8),  1);
    check("x9_lw",          reg(9),  8);
    check("x10_branch_taken_skip", reg(10), 42);   // would be 666 if beq mis-decoded
    check("x11_branch_not_taken",  reg(11), 77);
    check("x12_jal_link",          reg(12), 64);
    check("x13_jump_skip",         reg(13), 0);    // would be 555 if jal didn't skip addr 64
    check("x14_after_jump",        reg(14), 123);
    check("x15_lui",               reg(15), 0x12345000);
    check("x16_sltu",              reg(16), 1);
    check("x17_bne_taken_skip",    reg(17), 17);   // would be 888 if bne mis-decoded
    check("mem0_sw",               dmem(0), 8);
    check("final_pc",              pc(), 92);

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
