module tokenized_assignment_styles(
    input  logic clk,
    input  logic d,
    output logic q,
    output logic r
);
always_ff @(posedge clk)
    q
        = d;
always_comb
    r
        <= d;
endmodule
