module control_unit
    import alu_pkg::*;
(
    input  logic [6:0] opcode,
    input  logic [2:0] funct3,
    input  logic [6:0] funct7,

    output logic       reg_write,
    output logic       mem_read,
    output logic       mem_write,
    output logic       alu_src,     // 0 = rs2, 1 = immediate
    output logic       branch,
    output logic       branch_ne,   // 0 = beq, 1 = bne
    output logic       jump,
    output logic [1:0] wb_sel,      // 00=alu_result 01=mem_data 10=imm(lui) 11=pc+4(jal)
    output logic [3:0] alu_op
);
    always_comb begin
        // safe defaults — every signal gets a value on every path
        reg_write = 0; mem_read = 0; mem_write = 0; alu_src = 0;
        branch = 0; branch_ne = 0; jump = 0; wb_sel = 2'b00; alu_op = ALU_ADD;

        case (opcode)
            7'b0110011: begin // R-type
                reg_write = 1;
                case ({funct7, funct3})
                    10'b0000000_000: alu_op = ALU_ADD;
                    10'b0100000_000: alu_op = ALU_SUB;
                    10'b0000000_111: alu_op = ALU_AND;
                    10'b0000000_110: alu_op = ALU_OR;
                    10'b0000000_100: alu_op = ALU_XOR;
                    10'b0000000_001: alu_op = ALU_SLL;
                    10'b0000000_101: alu_op = ALU_SRL;
                    10'b0100000_101: alu_op = ALU_SRA;
                    10'b0000000_010: alu_op = ALU_SLT;
                    10'b0000000_011: alu_op = ALU_SLTU;
                    default: alu_op = ALU_ADD;
                endcase
            end

            7'b0010011: begin // I-type ALU: addi, slti, sltiu, xori, ori, andi, slli, srli, srai
                reg_write = 1;
                alu_src = 1;
                case (funct3)
                    3'b000: alu_op = ALU_ADD;                       // addi
                    3'b010: alu_op = ALU_SLT;                       // slti
                    3'b011: alu_op = ALU_SLTU;                      // sltiu
                    3'b100: alu_op = ALU_XOR;                       // xori
                    3'b110: alu_op = ALU_OR;                        // ori
                    3'b111: alu_op = ALU_AND;                       // andi
                    3'b001: alu_op = ALU_SLL;                       // slli
                    3'b101: alu_op = funct7[5] ? ALU_SRA : ALU_SRL; // srai : srli
                    default: alu_op = ALU_ADD;
                endcase
            end

            7'b0000011: begin // lw
                reg_write = 1;
                alu_src = 1;
                mem_read = 1;
                wb_sel = 2'b01;
                alu_op = ALU_ADD;
            end

            7'b0100011: begin // sw
                alu_src = 1;
                mem_write = 1;
                alu_op = ALU_ADD;
            end

            7'b1100011: begin // beq, bne
                branch = 1;
                branch_ne = (funct3 == 3'b001);
                alu_op = ALU_SUB;
            end

            7'b0110111: begin // lui
                reg_write = 1;
                wb_sel = 2'b10;
            end

            7'b1101111: begin // jal
                reg_write = 1;
                jump = 1;
                wb_sel = 2'b11;
            end

            default: ; // unrecognized opcode: defaults above hold (no-op)
        endcase
    end
endmodule
