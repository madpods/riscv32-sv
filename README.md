# RISC-V RV32I Core (SystemVerilog)

A RISC-V RV32I single-cycle CPU core written in SystemVerilog, built up
module by module and verified with [Verilator](https://www.veripool.org/verilator/).

Supports the R-type/I-type ALU ops, `lw`/`sw`, `beq`/`bne`, `lui`, and `jal`.
Not yet implemented: `jalr`, `blt`/`bge`/`bltu`/`bgeu`, `auipc`, `lb`/`lh`/`sb`/`sh`.

## Status

| Module | File | Status |
|---|---|---|
| Program Counter | [pc.sv](pc.sv) | Implemented — synchronous update, async active-low reset |
| Register File | [regfile.sv](regfile.sv) | Implemented — 32x32-bit regs, x0 hardwired to zero, sync write / combinational read |
| ALU | [alu.sv](alu.sv) | Implemented — add, sub, and, or, xor, sll, srl, sra, slt, sltu |
| Immediate Generator | [imm_gen.sv](imm_gen.sv) | Implemented — I/S/B/U/J formats |
| Control Unit | [control_unit.sv](control_unit.sv) | Implemented — decodes all currently-supported opcodes |
| Instruction Memory | [instr_mem.sv](instr_mem.sv) | Implemented — 1024x32-bit, loaded via `$readmemh` |
| Data Memory | [data_mem.sv](data_mem.sv) | Implemented — 1024x32-bit, read gated by `mem_read` |
| Top-level datapath | [riscv_single_cycle.sv](riscv_single_cycle.sv) | Implemented — wires the above into a single-cycle core |

## Goals

- Implement the RV32I base integer instruction set
- Keep each module small, testable, and simulated in isolation before integration
- Verify with a Verilator + C++ testbench per module, then a full-core testbench

## Simulation

Every module has its own Verilator C++ testbench (`tb_<module>.cpp`), built
and run independently, e.g.:

```sh
verilator --cc pc.sv --exe --build tb_pc.cpp -o Vpc_sim -Mdir obj_dir_pc
./obj_dir_pc/Vpc_sim
```

The full core is exercised end-to-end by `tb_riscv_single_cycle.cpp`, which
assembles a small RV32I program in C++, runs it through the datapath, and
checks the resulting register/memory/PC state:

```sh
verilator --cc alu_pkg.sv pc.sv regfile.sv alu.sv imm_gen.sv control_unit.sv \
  instr_mem.sv data_mem.sv riscv_single_cycle.sv --top-module riscv_single_cycle \
  --exe --build tb_riscv_single_cycle.cpp -o Vriscv_single_cycle_sim -Mdir obj_dir_top
./obj_dir_top/Vriscv_single_cycle_sim
```

`riscv_single_cycle.sv` exposes no ports beyond `clk`/`rst_n`, so the
integration testbench reads internal state (PC, register file, data memory)
through Verilator `/* verilator public */` accessors marked on those signals
— a test-only observability hook, not a functional change.

## Layout

```
alu_pkg.sv               - ALU opcode constants
alu.sv                   - Arithmetic/Logic Unit
imm_gen.sv                - Immediate generator (I/S/B/U/J formats)
control_unit.sv           - Opcode/funct3/funct7 decoder
pc.sv                      - Program counter register
regfile.sv                 - 32x32-bit general purpose register file
instr_mem.sv               - Instruction memory (loaded from a hex file)
data_mem.sv                - Data memory
riscv_single_cycle.sv       - Top-level single-cycle datapath
tb_*.cpp                    - Per-module Verilator testbenches
tb_riscv_single_cycle.cpp   - Full-core integration testbench
```

## Development Notes

Claude was used for writing code comments and Verilator testbench scaffolding.
All RTL module design and logic (PC, register file, ALU, and future
pipeline/hazard logic) is my own.
