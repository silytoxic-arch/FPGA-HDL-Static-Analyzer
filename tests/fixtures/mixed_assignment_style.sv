module mixed_assignment_style(
    input  logic clk,
    input  logic enable,
    input  logic d,
    output logic [1:0] q
);
always_ff @(posedge clk) begin
    if (enable) q[0] = d; // fpga-lint-disable-line VL-SEQ-BLOCKING
    else q[1] <= d;
end
endmodule
