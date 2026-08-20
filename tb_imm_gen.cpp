#include "Vimm_gen.h"
#include "verilated.h"
#include <cstdio>
#include <cstdint>

Vimm_gen* dut;
int errors = 0;

void check(const char* name, int32_t got, int32_t expected) {
    if (got != expected) {
        printf("FAIL [%s]: got=0x%08x expected=0x%08x\n", name, (uint32_t)got, (uint32_t)expected);
        errors++;
    } else {
        printf("PASS [%s]: 0x%08x\n", name, (uint32_t)got);
    }
}

// --- instruction-format builders, so expected values are derived from the
// same field layout the DUT decodes, rather than hand-computed hex literals ---

uint32_t make_itype(int32_t imm12, uint8_t rs1, uint8_t funct3, uint8_t rd, uint8_t opcode7) {
    uint32_t imm = (uint32_t)imm12 & 0xFFF;
    return (imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode7;
}

uint32_t make_stype(int32_t imm12, uint8_t rs2, uint8_t rs1, uint8_t funct3, uint8_t opcode7) {
    uint32_t imm = (uint32_t)imm12 & 0xFFF;
    uint32_t imm11_5 = (imm >> 5) & 0x7F;
    uint32_t imm4_0 = imm & 0x1F;
    return (imm11_5 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm4_0 << 7) | opcode7;
}

uint32_t make_btype(int32_t imm13, uint8_t rs2, uint8_t rs1, uint8_t funct3, uint8_t opcode7) {
    uint32_t imm = (uint32_t)imm13 & 0x1FFF; // bit0 always 0
    uint32_t b12 = (imm >> 12) & 1;
    uint32_t b11 = (imm >> 11) & 1;
    uint32_t b10_5 = (imm >> 5) & 0x3F;
    uint32_t b4_1 = (imm >> 1) & 0xF;
    return (b12 << 31) | (b10_5 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (b4_1 << 8) | (b11 << 7) | opcode7;
}

uint32_t make_utype(uint32_t imm20, uint8_t rd, uint8_t opcode7) {
    return ((imm20 & 0xFFFFF) << 12) | (rd << 7) | opcode7;
}

uint32_t make_jtype(int32_t imm21, uint8_t rd, uint8_t opcode7) {
    uint32_t imm = (uint32_t)imm21 & 0x1FFFFF; // bit0 always 0
    uint32_t b20 = (imm >> 20) & 1;
    uint32_t b19_12 = (imm >> 12) & 0xFF;
    uint32_t b11 = (imm >> 11) & 1;
    uint32_t b10_1 = (imm >> 1) & 0x3FF;
    return (b20 << 31) | (b10_1 << 21) | (b11 << 20) | (b19_12 << 12) | (rd << 7) | opcode7;
}

const uint8_t OP_I_ALU  = 0b0010011;
const uint8_t OP_LOAD   = 0b0000011;
const uint8_t OP_STORE  = 0b0100011;
const uint8_t OP_BRANCH = 0b1100011;
const uint8_t OP_LUI    = 0b0110111;
const uint8_t OP_JAL    = 0b1101111;

void run(const char* name, uint32_t instr, int32_t expected) {
    dut->instr = instr;
    dut->eval();
    check(name, dut->imm, expected);
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    dut = new Vimm_gen;

    // I-type: addi-style (also covers loads, same imm[11:0] field)
    run("itype_pos", make_itype(5, 0, 0, 0, OP_I_ALU), 5);
    run("itype_neg", make_itype(-1, 0, 0, 0, OP_I_ALU), -1);
    run("load_itype", make_itype(-4, 1, 0b010, 2, OP_LOAD), -4);

    // S-type: sw
    run("stype_pos", make_stype(100, 3, 1, 0b010, OP_STORE), 100);
    run("stype_neg", make_stype(-4, 3, 1, 0b010, OP_STORE), -4);

    // B-type: beq/bne (imm always even — LSB forced to 0 in the encoding)
    run("btype_pos", make_btype(8, 2, 1, 0b000, OP_BRANCH), 8);
    run("btype_neg", make_btype(-2, 2, 1, 0b001, OP_BRANCH), -2);

    // U-type: lui (low 12 bits always zero)
    run("utype", make_utype(0x12345, 5, OP_LUI), 0x12345000);

    // J-type: jal (imm always even)
    run("jtype_pos", make_jtype(16, 1, OP_JAL), 16);
    run("jtype_neg", make_jtype(-4, 1, OP_JAL), -4);

    // Unrecognized opcode -> defaults to 0
    run("default_opcode", 0x00000033 /* R-type, opcode=0110011 */, 0);

    delete dut;

    if (errors == 0) {
        printf("\nAll tests PASSED\n");
        return 0;
    } else {
        printf("\n%d test(s) FAILED\n", errors);
        return 1;
    }
}
