# GJB10157.8.7 ADD/SUB concat evidence

Target: `T-Linter.dll`  
SHA-256: `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`

Evidence type: Ghidra 12.1.4 static pseudocode plus upstream Yosys source-layout
corroboration. This is not runtime observation.

## Binary observations

In `FUN_1805790d0` (`reverse-tools/evidence/t_linter_pass4/gjb10157_8_7_core.txt`):

1. Cases `0x3c/0x3d` iterate every direct ADD/SUB child and call
   `FUN_1808f4650` to infer its width.
2. For an operand of type `0x22` with more than one child, the function copies
   the three-pointer vector at `last_child + 0x88` and records the operand index
   iff the vector is empty or every byte is zero.
3. During the width-map pass, width greater than the target reports
   immediately. Width equal to the target reports only when the operand index
   was not recorded by step 2. Width below the target does not report.
4. There is no identifier-name equality test in this ADD/SUB helper.

In `FUN_1808f46c0`
(`reverse-tools/evidence/t_linter_pass4/expression_width_helper.txt`), case
`0x10` computes the AST constant width from the vector bounds at `+0x88` and
`+0x90`. This anchors the inspected vector as the constant bit-state vector.

## Upstream layout/order corroboration

The official Yosys `AstNode` definition places `std::vector<RTLIL::State> bits`
after `str`, and official `RTLIL::State` defines `S0=0`, `S1=1`, `Sx=2`,
`Sz=3`. The official Verilog `concat_list` grammar recursively creates the
right-hand list and appends the left expression, so the final AST child is the
source-leftmost concat item.

- <https://github.com/YosysHQ/yosys/blob/main/frontends/ast/ast.h>
- <https://github.com/YosysHQ/yosys/blob/main/frontends/verilog/verilog_parser.y>
- <https://github.com/YosysHQ/yosys/blob/main/kernel/rtlil.h>

The upstream source is corroboration for field meaning and child order; the
shipped DLL pseudocode remains the authority for the rule predicate.

## Recovered predicate

For every direct ADD/SUB operand with inferred width `w` and assignment target
width `t`:

- report if `w > t`;
- if `w == t`, report unless the operand is a concat with more than one direct
  child and its source-leftmost child's bits vector is empty or entirely `S0`;
- do not report for that operand if `w < t`.

An empty bits vector passes exactly as an all-zero vector in the DLL loop. This
means a supported identifier/select as the source-leftmost concat item also
receives the native exemption, even though that is broader than a semantic
"zero extension" rule.

## Remaining unknowns

Full `FUN_1808f4650` inference for nested arithmetic and all SystemVerilog
expression forms, parameter overrides, cell-port target-width propagation,
signedness interactions, AST rewrite phase and diagnostic deduplication remain
unverified. Product support must remain bounded to statically provable shapes.
