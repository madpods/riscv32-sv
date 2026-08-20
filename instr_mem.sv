module instr_mem (
    input  logic [31:0] addr,
    output logic [31:0] instr
);
    logic [31:0] mem [0:1023];  // 1024 words = 4KB of instruction space

    initial $readmemh("program.hex", mem);  // loads a hex file at sim start

    assign instr = mem[addr[11:2]];
endmodule
