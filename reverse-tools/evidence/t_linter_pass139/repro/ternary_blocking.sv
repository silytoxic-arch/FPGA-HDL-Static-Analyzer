module m(input clk,input [3:0] cond,output reg y);always @(posedge clk)y=cond?1'b1:1'b0;endmodule
