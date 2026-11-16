module duplicate_sensitivity(
    input  logic a,
    input  logic b,
    output logic q
);
always @(a or a or b) q = a & b;
endmodule
