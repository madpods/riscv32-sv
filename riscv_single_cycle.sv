module riscv_single_cycle (
    input logic clk,
    input logic rst_n
);
    logic [31:0] pc_current, pc_next, pc_plus4, branch_target, jump_target;
    logic [31:0] instr, rs1_data, rs2_data, alu_b, alu_result, imm, mem_rdata, wb_data;
    logic [4:0]  rs1_addr, rs2_addr, rd_addr;
    logic [6:0]  opcode, funct7;
    logic [2:0]  funct3;
    logic        reg_write, mem_read, mem_write, alu_src, branch, branch_ne, jump, zero, branch_taken;
    logic [1:0]  wb_sel;
    logic [3:0]  alu_op;

    pc pc_reg (.clk, .rst_n, .pc_next, .pc_current);
    instr_mem imem (.addr(pc_current), .instr);

    assign opcode   = instr[6:0];
    assign funct3   = instr[14:12];
    assign funct7   = instr[31:25];
    assign rs1_addr = instr[19:15];
    assign rs2_addr = instr[24:20];
    assign rd_addr  = instr[11:7];

    control_unit cu (.opcode, .funct3, .funct7, .reg_write, .mem_read, .mem_write,
                      .alu_src, .branch, .branch_ne, .jump, .wb_sel, .alu_op);

    regfile rf (.clk, .rst_n, .we(reg_write), .rs1_addr, .rs2_addr, .rd_addr,
                .rd_data(wb_data), .rs1_data, .rs2_data);

    imm_gen ig (.instr, .imm);

    assign alu_b = alu_src ? imm : rs2_data;
    alu alu_inst (.a(rs1_data), .b(alu_b), .alu_op, .result(alu_result), .zero);

    data_mem dmem (.clk, .addr(alu_result), .write_data(rs2_data),
                    .mem_read, .mem_write, .read_data(mem_rdata));

    assign pc_plus4      = pc_current + 32'd4;
    assign branch_target = pc_current + imm;
    assign jump_target   = pc_current + imm;
    assign branch_taken  = branch & (branch_ne ? ~zero : zero);
    assign pc_next        = jump ? jump_target : (branch_taken ? branch_target : pc_plus4);

    always_comb begin
        case (wb_sel)
            2'b00:   wb_data = alu_result;   // add, sub, addi...
            2'b01:   wb_data = mem_rdata;    // lw
            2'b10:   wb_data = imm;          // lui
            2'b11:   wb_data = pc_plus4;     // jal
            default: wb_data = alu_result;
        endcase
    end
endmodule
