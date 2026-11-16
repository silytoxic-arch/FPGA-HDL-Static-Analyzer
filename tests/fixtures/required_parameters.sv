module required_parameter_child #(
    parameter WIDTH = 8,
    parameter type VALUE_T
)(
    input  logic a,
    output logic y
);
    assign y = a;
endmodule

module required_parameter_top(
    input  logic a,
    output logic y
);
    required_parameter_child u_missing(.a(a), .y(y));
    required_parameter_child #(.WIDTH(16)) u_named_missing(.a(a), .y(y));
    required_parameter_child #(.VALUE_T(logic)) u_named_ok(.a(a), .y(y));
    required_parameter_child #(8, logic) u_positional_ok(.a(a), .y(y));
    required_parameter_child #(8, logic, 3) u_many(.a(a), .y(y));
    required_parameter_child #(8, .VALUE_T(logic)) u_mixed(.a(a), .y(y));
endmodule
