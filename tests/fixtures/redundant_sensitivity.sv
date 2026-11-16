module redundant_sensitivity(
    input  logic a,
    input  logic b,
    input  logic spare,
    output logic q
);
always @(a or b or spare) q = a & b;
endmodule
