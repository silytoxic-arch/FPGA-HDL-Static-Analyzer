`define MAKE module child(input [3:0] d);endmodule
`MAKE
module parent(input [1:0] s);child u(.d(s));endmodule
