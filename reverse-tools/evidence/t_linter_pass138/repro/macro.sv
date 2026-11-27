`define MAKE module condition_owner(input [3:0] cond,output reg y);always @* if(cond) y=1;else y=0;endmodule
`MAKE
