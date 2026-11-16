module sensitivity_implicit(input A, B, output reg C, output reg Q);
always @(A, CLK) C = A;
always @(A) if (B_COND) C = A & B;
always @(B_COND) C = A;
always @(posedge CLK) Q <= A;
endmodule
