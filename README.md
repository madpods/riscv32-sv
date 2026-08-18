# RISC-V RV32I Core (SystemVerilog)

A RISC-V RV32I CPU core written in SystemVerilog, built up module by module and
verified with [Verilator](https://www.veripool.org/verilator/).

This is a work in progress — modules are being implemented and tested
incrementally rather than delivered as a finished core.

## Status

| Module | File | Status |
|---|---|---|
| Program Counter | [pc.sv](pc.sv) | Implemented — synchronous update, async active-low reset |
| Register File | [regfile.sv](regfile.sv) | Implemented — 32x32-bit regs, x0 hardwired to zero, sync write / combinational read |
| ALU | [alu.sv](alu.sv) | Stub — port list only, logic not yet written |
| Instruction Decode | — | Not started |
| Control Unit | — | Not started |
| Memory / Bus | — | Not started |

## Goals

- Implement the RV32I base integer instruction set
- Keep each module small, testable, and simulated in isolation before integration
- Verify with a Verilator + C++ testbench per module, then a full-core testbench

## Simulation

Each module has its own Verilator C++ testbench (`tb_<module>.cpp`), built and
run independently:

```sh
verilator --cc pc.sv --exe --build tb_pc.cpp -o Vpc_sim -Mdir obj_dir_pc
./obj_dir_pc/Vpc_sim

verilator --cc regfile.sv --exe --build tb_regfile.cpp -o Vregfile_sim -Mdir obj_dir_regfile
./obj_dir_regfile/Vregfile_sim
```

## Layout

```
alu.sv          - Arithmetic/Logic Unit (stub)
pc.sv           - Program counter register
regfile.sv      - 32x32-bit general purpose register file
tb_pc.cpp       - Verilator testbench for pc.sv
tb_regfile.cpp  - Verilator testbench for regfile.sv
```
