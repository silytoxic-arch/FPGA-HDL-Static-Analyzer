module child #(parameter P=4)(input [P-1:0] a,b,output reg y);always @* y=a&&b;endmodule
module parent(input a,b,output y);child #(.P(1)) inst(a,b,y);endmodule
