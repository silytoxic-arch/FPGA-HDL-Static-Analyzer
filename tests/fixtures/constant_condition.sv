module constant_condition(
    input  logic clk,
    input  logic d,
    output logic q
);
always_ff @(posedge clk) begin
    if (1'b0) q <= d;
    else q <= d;
end
endmodule
