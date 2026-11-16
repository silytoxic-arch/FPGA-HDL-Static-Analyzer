module unused_parameter #(
    parameter USED = 2,
    parameter UNUSED = 1
)(
    input  logic [USED-1:0] d,
    output logic q
);
assign q = ^d;
endmodule
