#include "Vcontrol_unit.h"
#include "verilated.h"
#include <cstdio>
#include <cstdint>
#include <string>

Vcontrol_unit* dut;
int errors = 0;

// Mirrors alu_pkg.sv's localparams — kept in sync manually since the
// testbench can't import the SystemVerilog package directly.
enum { ALU_ADD=0, ALU_SUB=1, ALU_AND=2, ALU_OR=3, ALU_XOR=4,
       ALU_SLL=5, ALU_SRL=6, ALU_SRA=7, ALU_SLT=8, ALU_SLTU=9 };

struct Expected {
    uint8_t reg_write, mem_read, mem_write, alu_src;
    uint8_t branch, branch_ne, jump, wb_sel, alu_op;
};

void check(const char* name, uint8_t got, uint8_t expected, const char* field) {
    if (got != expected) {
        printf("FAIL [%s.%s]: got=%u expected=%u\n", name, field, got, expected);
        errors++;
    }
}

void run(const char* name, uint8_t opcode, uint8_t funct3, uint8_t funct7, const Expected& e) {
    dut->opcode = opcode;
    dut->funct3 = funct3;
    dut->funct7 = funct7;
    dut->eval();

    int before = errors;
    check(name, dut->reg_write,  e.reg_write,  "reg_write");
    check(name, dut->mem_read,   e.mem_read,   "mem_read");
    check(name, dut->mem_write,  e.mem_write,  "mem_write");
    check(name, dut->alu_src,    e.alu_src,    "alu_src");
    check(name, dut->branch,     e.branch,     "branch");
    check(name, dut->branch_ne,  e.branch_ne,  "branch_ne");
    check(name, dut->jump,       e.jump,       "jump");
    check(name, dut->wb_sel,     e.wb_sel,     "wb_sel");
    check(name, dut->alu_op,     e.alu_op,     "alu_op");

    if (errors == before) printf("PASS [%s]\n", name);
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    dut = new Vcontrol_unit;

    const uint8_t R_TYPE  = 0b0110011;
    const uint8_t I_ALU   = 0b0010011;
    const uint8_t LOAD    = 0b0000011;
    const uint8_t STORE   = 0b0100011;
    const uint8_t BRANCH  = 0b1100011;
    const uint8_t LUI     = 0b0110111;
    const uint8_t JAL     = 0b1101111;

    // R-type: reg_write=1, alu_src=0, everything else off, alu_op selected by {funct7,funct3}
    run("add",  R_TYPE, 0b000, 0b0000000, {1,0,0,0, 0,0,0, 0b00, ALU_ADD});
    run("sub",  R_TYPE, 0b000, 0b0100000, {1,0,0,0, 0,0,0, 0b00, ALU_SUB});
    run("and",  R_TYPE, 0b111, 0b0000000, {1,0,0,0, 0,0,0, 0b00, ALU_AND});
    run("or",   R_TYPE, 0b110, 0b0000000, {1,0,0,0, 0,0,0, 0b00, ALU_OR});
    run("xor",  R_TYPE, 0b100, 0b0000000, {1,0,0,0, 0,0,0, 0b00, ALU_XOR});
    run("sll",  R_TYPE, 0b001, 0b0000000, {1,0,0,0, 0,0,0, 0b00, ALU_SLL});
    run("srl",  R_TYPE, 0b101, 0b0000000, {1,0,0,0, 0,0,0, 0b00, ALU_SRL});
    run("sra",  R_TYPE, 0b101, 0b0100000, {1,0,0,0, 0,0,0, 0b00, ALU_SRA});
    run("slt",  R_TYPE, 0b010, 0b0000000, {1,0,0,0, 0,0,0, 0b00, ALU_SLT});
    run("sltu", R_TYPE, 0b011, 0b0000000, {1,0,0,0, 0,0,0, 0b00, ALU_SLTU});

    // I-type ALU: reg_write=1, alu_src=1, alu_op selected by funct3 (+funct7[5] for shifts)
    run("addi",  I_ALU, 0b000, 0b0000000, {1,0,0,1, 0,0,0, 0b00, ALU_ADD});
    run("slti",  I_ALU, 0b010, 0b0000000, {1,0,0,1, 0,0,0, 0b00, ALU_SLT});
    run("sltiu", I_ALU, 0b011, 0b0000000, {1,0,0,1, 0,0,0, 0b00, ALU_SLTU});
    run("xori",  I_ALU, 0b100, 0b0000000, {1,0,0,1, 0,0,0, 0b00, ALU_XOR});
    run("ori",   I_ALU, 0b110, 0b0000000, {1,0,0,1, 0,0,0, 0b00, ALU_OR});
    run("andi",  I_ALU, 0b111, 0b0000000, {1,0,0,1, 0,0,0, 0b00, ALU_AND});
    run("slli",  I_ALU, 0b001, 0b0000000, {1,0,0,1, 0,0,0, 0b00, ALU_SLL});
    run("srli",  I_ALU, 0b101, 0b0000000, {1,0,0,1, 0,0,0, 0b00, ALU_SRL});
    run("srai",  I_ALU, 0b101, 0b0100000, {1,0,0,1, 0,0,0, 0b00, ALU_SRA});

    // lw: reg_write, alu_src, mem_read, wb_sel=01 (mem_data)
    run("lw", LOAD, 0b010, 0b0000000, {1,1,0,1, 0,0,0, 0b01, ALU_ADD});

    // sw: alu_src, mem_write, no reg_write
    run("sw", STORE, 0b010, 0b0000000, {0,0,1,1, 0,0,0, 0b00, ALU_ADD});

    // beq / bne: branch=1, branch_ne differentiates, alu_op=SUB
    run("beq", BRANCH, 0b000, 0b0000000, {0,0,0,0, 1,0,0, 0b00, ALU_SUB});
    run("bne", BRANCH, 0b001, 0b0000000, {0,0,0,0, 1,1,0, 0b00, ALU_SUB});

    // lui: reg_write, wb_sel=10 (imm)
    run("lui", LUI, 0b000, 0b0000000, {1,0,0,0, 0,0,0, 0b10, ALU_ADD});

    // jal: reg_write, jump, wb_sel=11 (pc+4)
    run("jal", JAL, 0b000, 0b0000000, {1,0,0,0, 0,0,1, 0b11, ALU_ADD});

    // unrecognized opcode: everything falls back to the pre-case defaults
    run("unrecognized_opcode", 0b0000000, 0, 0, {0,0,0,0, 0,0,0, 0b00, ALU_ADD});

    delete dut;

    if (errors == 0) {
        printf("\nAll tests PASSED\n");
        return 0;
    } else {
        printf("\n%d check(s) FAILED\n", errors);
        return 1;
    }
}
