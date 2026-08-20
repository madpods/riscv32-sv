module data_mem (
    input  logic        clk,
    input  logic [31:0] addr,
    input  logic [31:0] write_data,
    input  logic        mem_read,
    input  logic        mem_write,
    output logic [31:0] read_data
);
    logic [31:0] mem [0:1023];

    assign read_data = mem_read ? mem[addr[11:2]] : 32'h0;

    always_ff @(posedge clk) begin
        if (mem_write)
            mem[addr[11:2]] <= write_data;
    end
endmodule
