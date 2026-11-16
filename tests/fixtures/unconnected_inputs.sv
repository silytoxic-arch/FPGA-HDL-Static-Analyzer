module required_child(
    input  logic required_a,
    input  logic required_b,
    input  logic optional_c = 1'b0,
    output logic y
);
    assign y = required_a & required_b & optional_c;
endmodule

module required_top(
    input  logic a,
    input  logic required_b,
    output logic y
);
    required_child u_missing(.required_a(a), .y(y));
    required_child u_empty(.required_a(a), .required_b(), .y(y));
    required_child u_wildcard(.*);
    required_child u_shorthand(.required_a(a), .required_b, .y);
endmodule
