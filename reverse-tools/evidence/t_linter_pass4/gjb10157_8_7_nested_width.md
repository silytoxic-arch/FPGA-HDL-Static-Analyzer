# GJB10157.8.7 nested expression width evidence

Target: `T-Linter.dll`  
SHA-256: `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`

Evidence type: Ghidra 12.1.4 static pseudocode and decoded x86-64
instructions. This is not runtime observation.

## Wrapper and call-mode observations

`FUN_1808f4650` initializes the output width to `-1`, signedness to true, clears
the optional status byte, forwards the caller's fifth byte argument to
`FUN_1808f46c0`, and rejects a resulting width greater than or equal to
`0x1000000`.

The decoded instructions at all five GJB10157.8.7 calls inside
`FUN_1805790d0` write zero to `[RSP+0x20]` immediately before calling
`FUN_1808f4650`. On the Windows x64 ABI that stack slot is the fifth argument.
Retained instruction evidence:

- `expression_width_wrapper_asm.txt`
- `gjb10157_8_7_width_call_asm.txt`

The latter includes the ADD/SUB call at `0x18057918f`, MUL call at
`0x180579679`, DIV/MOD call at `0x1805796c0`, and POW operand calls at
`0x18057970a` and `0x180579728`.

## `FUN_1808f46c0` flag-zero width branches

Full pseudocode is retained in `expression_width_helper.txt`.

- `AST_CONSTANT(0x10)`: bit-vector length.
- `AST_IDENTIFIER(0x13)`: resolved declaration/selection width.
- `AST_CONCAT(0x22)`: sum of every child width.
- `AST_REPLICATE(0x23)`: constant repeat count multiplied by repeated child
  width.
- `AST_ADD/SUB/DIV/MOD(0x3c/0x3d/0x3f/0x40)`: recurse over all children while
  retaining the maximum width.
- `AST_MUL(0x3e)`: with the fifth flag zero, recurse over all children while
  retaining the maximum width. Its alternate sum-like path is not selected by
  this rule's calls.
- `AST_POW(0x42)`, `AST_POS(0x43)` and `AST_NEG(0x44)`: propagate the first
  child width.

This helper behavior is distinct from the outer rule switch:

- an outer MUL sums the already inferred widths of its direct children;
- an outer DIV/MOD compares `max(child widths)-1`;
- an outer POW uses the separate exponential formula and AST_IDENTIFIER base
  gate;
- an outer ADD/SUB applies its per-direct-child width and concat exemption.

For example, in left-associated `A*B*C`, the outer MUL sees child `(A*B)` and
child `C`; the nested `(A*B)` width is `max(width(A),width(B))`, so the outer
threshold is that maximum plus `width(C)`, not the sum of all three leaves.

## Product boundary

The independent implementation now applies these branches to recursively
parsed arithmetic composed of statically sized identifiers/selections,
explicitly sized literals, concat/replicate nodes, parentheses and unary
plus/minus. Other AST operators, unresolved parameters, functions, dynamic
selects, malformed expressions, and widths reaching the native implementation
limit remain unsupported and do not certify compliance.
