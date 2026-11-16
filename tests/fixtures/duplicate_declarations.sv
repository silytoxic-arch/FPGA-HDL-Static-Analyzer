module duplicate_interface #(
    parameter WIDTH = 8,
    parameter WIDTH = 16
)(
    input  logic a,
    input  logic a,
    output logic y
);
    assign y = a;
endmodule
