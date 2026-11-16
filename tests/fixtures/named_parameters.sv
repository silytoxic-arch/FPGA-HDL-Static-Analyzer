module parameter_child #(
    parameter integer WIDTH = 8,
    parameter type VALUE_T = logic,
    parameter DEPTH = 4
)(
    input  logic a,
    output logic y
);
    assign y = a;
endmodule

module no_parameter_child(
    input  logic a,
    output logic y
);
    assign y = a;
endmodule

module parameter_top(
    input  logic a,
    output logic y
);
    parameter_child #(
        .WIDTH(16),
        .TYPO(2),
        .WIDTH(32)
    ) u_bad(.a(a), .y(y));

    parameter_child #(16, logic, 4) u_positional(.a(a), .y(y));
    no_parameter_child #(.GHOST(1)) u_none(.a(a), .y(y));
endmodule
