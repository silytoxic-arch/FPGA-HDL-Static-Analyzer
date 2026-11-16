module systemverilog_process_controls(
    input  logic clk,
    input  logic clk2,
    input  logic a,
    output logic q_comb,
    output logic q_missing,
    output logic q_multiple,
    output logic q_ok,
    output logic q_latch,
    output logic q_latch_ok,
    output logic q_no_latch
);
    always_comb @(a) q_comb = a;
    always_ff begin q_missing <= a; end
    always_ff @(posedge clk) @(posedge clk2) q_multiple <= a;
    always_ff @(posedge clk) q_ok <= a;
    always_latch @(a) if (a) q_latch = 1'b1;
    always_latch if (a) q_latch_ok = 1'b1;
    always_latch if (a) q_no_latch = 1'b1; else q_no_latch = 1'b0;
endmodule
