# Bug fixes — 2026-08-19

Found by running `verilator --lint-only -Wall` across the whole design
(`alu_pkg.sv alu.sv pc.sv regfile.sv imm_gen.sv control_unit.sv instr_mem.sv
data_mem.sv riscv_single_cycle.sv`, top module `riscv_single_cycle`) and a
manual read-through of `control_unit.sv` against the RV32I opcode/funct3/funct7
tables. Five issues, one of them a real correctness bug.

## 1. `control_unit.sv` — SLTU missing from the R-type decode

**Before:** the `{funct7, funct3}` case for R-type instructions had entries
for `add`, `sub`, `and`, `or`, `xor`, `sll`, `srl`, `sra`, `slt`, but not
`sltu` (`funct7=0000000, funct3=011`). That encoding fell through to
`default: alu_op = ALU_ADD`.

**Why it mattered:** `alu.sv` already implements `ALU_SLTU` correctly — the
ALU was fine. The bug was upstream: the control unit never produced that
opcode, so any `sltu` instruction would silently execute as `add` instead of
an unsigned comparison. Wrong result, no error, no warning at simulation
time — exactly the kind of bug that only shows up as a mysterious test
failure much later.

**Fix:** added `10'b0000000_011: alu_op = ALU_SLTU;` to the R-type case.

## 2. `control_unit.sv` — I-type ALU decode only covered 3 of 9 ops

**Before:** the I-type ALU case (`opcode 7'b0010011`) only decoded
`addi`, `andi`, `ori`. `slti`, `sltiu`, `xori`, `slli`, `srli`, `srai` all
fell through to the same `default: alu_op = ALU_ADD`.

**Why it mattered:** same failure mode as #1, just five instructions wide
instead of one — silent wrong-answer execution instead of a build error,
which is worse than a crash because nothing points you at the bug.

**Fix:** decode all nine I-type ALU `funct3` values. `slli`/`srli`/`srai`
share `funct3` with their R-type counterparts, and RV32I reuses the same bit
position (`instr[30]`, i.e. `funct7[5]`) to tell `srli` from `srai` in the
I-type encoding, so the fix reuses that bit rather than inventing new
control logic:

```systemverilog
3'b101: alu_op = funct7[5] ? ALU_SRA : ALU_SRL; // srai : srli
```

No change was needed to route the shift amount — `imm_gen` already produces
a sign-extended immediate whose low 5 bits equal `instr[24:20]` (the `shamt`
field), and the ALU's shift ops already mask to `b[4:0]`, so `alu_src`
selecting the immediate as ALU input `b` was already correct for shifts.

## 3. `control_unit.sv` — outer `case (opcode)` had no `default`

**Before:** the top-level `case (opcode)` enumerated the seven opcodes this
core supports with no `default:` arm. Verilator's `CASEINCOMPLETE` warning
flagged it.

**Why it mattered:** functionally this was harmless — every control signal
is pre-assigned a safe value (`reg_write = 0`, `alu_op = ALU_ADD`, etc.)
before the `case` runs, so an unrecognized opcode already fell back to a
no-op. But that safety is only visible by reading the whole `always_comb`
block; Verilator (and a future reader skimming just the `case`) can't prove
it statically, so it's flagged as a possible incomplete-case bug on every
lint run, burying real warnings under a false positive.

**Fix:** added `default: ; // unrecognized opcode: defaults above hold
(no-op)` to make the intent explicit and silence the warning.

## 4. `instr_mem.sv` / `data_mem.sv` — implicit address truncation

**Before:** both memories are 1024 x 32-bit (10-bit index) but were indexed
with the full `addr[31:2]` (30 bits): `mem[addr[31:2]]`. Verilator's
`WIDTHTRUNC` warning caught this — it silently truncates to the low 10 bits
at simulation time.

**Why it mattered:** the simulated behavior was already "correct" in the
sense that Verilator's implicit truncation happens to keep the low bits,
which is what you want. But relying on an *implicit* truncation to get the
*intended* behavior means any address above the 4KB range silently wraps
around and aliases into the same 1024 words instead of raising any kind of
warning or error at the point of use — a future out-of-bounds access (e.g. a
runaway PC, or a `lw`/`sw` with a bad computed address) would fail silently
by reading/writing the wrong word instead of failing loudly.

**Fix:** sliced explicitly to `mem[addr[11:2]]` in both modules, matching
the actual 1024-word address range and removing the implicit-width
dependency.

## 5. `data_mem.sv` — `mem_read` input was unused

**Before:** `read_data` was driven straight from `mem[addr[11:2]]`
unconditionally; the `mem_read` input existed in the port list but was never
read anywhere in the module body (Verilator's `UNUSEDSIGNAL` warning).

**Why it mattered:** not a functional bug in the current single-cycle
datapath — `riscv_single_cycle.sv` only routes `mem_rdata` into `wb_data`
when `wb_sel` calls for it (set by `control_unit` alongside `mem_read`), so
non-load instructions never observed the always-on read. But a real memory
shouldn't drive its output bus independent of its own read-enable — leaving
`mem_read` unused is the kind of dangling port that tends to hide a real bug
later, once something else (e.g. a pipelined version, or a shared bus) reads
`read_data` without going through `wb_sel`.

**Fix:** gated the read: `assign read_data = mem_read ? mem[addr[11:2]] :
32'h0;`.

## Verification

Re-ran `verilator --lint-only -Wall` after the fixes: all five warnings
above are gone. Two `UNUSEDSIGNAL` warnings remain on `addr[31:12,1:0]` in
both memory modules — expected and left as-is, since only `addr[11:2]`
(the word index within the 4KB region) is meaningful for a 1024-word memory;
the high bits and byte-offset bits are unused by design, not by mistake.

`tb_pc.cpp` and `tb_regfile.cpp` were not affected by these changes (neither
`pc.sv` nor `regfile.sv` was touched) and continue to pass.
