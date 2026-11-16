module positional_required_child(
    input  logic a,
    input  logic b,
    input  logic optional_c = 1'b0,
    output logic y
);
    assign y = a & b & optional_c;
endmodule

module positional_required_top(
    input  logic a,
    input  logic b,
    output logic y
);
    positional_required_child u_missing(a);
    positional_required_child u_empty(a, , , y);
    positional_required_child u_ok(a, b);
    positional_required_child u_many(a, b, 1'b0, y, a);
endmodule
