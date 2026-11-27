`define MAKE module clocked_owner(input clk,input d,output reg q);always @(posedge clk) q<=d;endmodule
`MAKE
