module m(input a,input [3:0] b,output reg y);always @* if(!a+b)y=1;else y=0;endmodule
