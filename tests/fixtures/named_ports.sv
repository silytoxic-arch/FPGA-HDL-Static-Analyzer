module interface_child(input logic clk, input logic [7:0] data_i,
                       output logic valid_o);
assign valid_o = clk & data_i[0];
endmodule

module interface_top(input logic clk, input logic [7:0] data,
                     output logic valid);
interface_child u_bad(.clk(clk), .data_typo(data), .valid_o(valid), .clk(clk));
endmodule
