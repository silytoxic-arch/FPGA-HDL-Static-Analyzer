# GJB10157.8.1 flag-zero width calls

Target: `T-Linter.dll`

SHA-256: `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`

Evidence type: static Ghidra 12.1.4 decoded instructions; no native program was
executed. Full retained range: `gjb10157_8_1_asm.txt`.

The two width calls for a selected two-child comparison node are at
`0x1805706ad` and `0x1805706cc`. Immediately before each call, the function
writes `0` to `[RSP+0x20]` (`0x180570698` and `0x1805706b6`). Under the Windows
x64 ABI this stack slot supplies the fifth argument to `FUN_1808f4650`.

Observed call sequence:

```text
180570698 MOV byte ptr [RSP + 0x20],0x0
18057069d XOR R9D,R9D
1805706a0 LEA R8,[RSP + 0x30]
1805706a5 LEA RDX,[RSP + 0x38]
1805706aa MOV RCX,qword ptr [RCX]
1805706ad CALL 0x1808f4650

1805706b6 MOV byte ptr [RSP + 0x20],0x0
1805706bb XOR R9D,R9D
1805706be LEA R8,[RSP + 0x30]
1805706c3 LEA RDX,[RSP + 0x34]
1805706c8 MOV RCX,qword ptr [RCX + 0x8]
1805706cc CALL 0x1808f4650
1805706d1 MOV EAX,dword ptr [RSP + 0x34]
1805706d5 CMP dword ptr [RSP + 0x38],EAX
```

Combined with retained `expression_width_helper.txt`, the flag-zero path means
that nested ADD/SUB/MUL/DIV/MOD operands take the maximum child width, POW and
unary signs propagate the first child, CONCAT sums its child widths, and
REPLICATE multiplies a static repeat count by the repeated child width.

The same function's earlier collector evidence admits AST_ALWAYS and
AST_ASSIGN roots, recursively gathers AST_LT/LE/GE/GT/EQ/NE/EQX/NEX nodes, and
the shown compare reports only unequal inferred widths. Product mapping is
therefore bounded to parser-confirmed always/procedural assignments,
continuous assignments and supported static-width expressions. Unsupported
operators, calls, dynamic selections, unresolved parameters/macros, generated
nodes and exact native diagnostic order/multiplicity remain unknown.
