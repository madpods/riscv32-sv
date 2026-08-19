module imm_gen (
    input  logic [31:0] instr,
    output logic [31:0] imm
);
    logic [6:0] opcode;
    assign opcode = instr[6:0];

    always_comb begin
        case (opcode)
            7'b0010011, // I-type ALU (addi, andi, ori, slti...)
            7'b0000011: // I-type load (lw)
                imm = {{20{instr[31]}}, instr[31:20]};

            7'b0100011: // S-type (sw)
                imm = {{20{instr[31]}}, instr[31:25], instr[11:7]};

            7'b1100011: // B-type (beq, bne)
                imm = {{19{instr[31]}}, instr[31], instr[7], instr[30:25], instr[11:8], 1'b0};

            7'b0110111: // U-type (lui)
                imm = {instr[31:12], 12'b0};

            7'b1101111: // J-type (jal)
                imm = {{11{instr[31]}}, instr[31], instr[19:12], instr[20], instr[30:21], 1'b0};

            default:
                imm = 32'h0;
        endcase
    end
endmodule
