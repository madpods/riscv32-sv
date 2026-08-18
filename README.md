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

Modules are simulated with [Verilator](https://www.veripool.org/verilator/).
Example (once a module's testbench is in place):

```sh
verilator --cc <module>.sv --exe sim_main.cpp --build
./obj_dir/V<module>
```

## Layout

```
alu.sv        - Arithmetic/Logic Unit (stub)
pc.sv         - Program counter register
regfile.sv    - 32x32-bit general purpose register file
sim_main.cpp  - Verilator C++ testbench driver
```
