module probe(
 input CLK,
 output C2,
 input A,
 input B,
 inout D,
 input B2,
 output reg C
); always @(posedge CLK) C <= A; endmodule
