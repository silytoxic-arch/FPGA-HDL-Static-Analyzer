module concatenation_width(input logic [3:0] a, input logic [1:0] b, input logic [1:0] idx,
  output logic [3:0] q0, q2, output logic [5:0] q1, output logic [3:0] q3);
always_comb begin
  q0 = {a, b};
  q1 = {a, b};
  q2 = {{2'b01}, b};
  q3[1:0] = {a[1:0], b};
  q3 = {2{b}};
  q2 = {3{b}};
  q2 = {a[idx], b};
end
endmodule
