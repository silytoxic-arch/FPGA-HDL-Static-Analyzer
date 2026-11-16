module self_assignment(
    input  logic clk,
    output logic q
);
always_ff @(posedge clk) q <= q;
endmodule
