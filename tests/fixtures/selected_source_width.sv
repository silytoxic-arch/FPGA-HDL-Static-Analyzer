module selected_source_width(input logic [15:0] data, input logic [3:0] idx,
  output logic [3:0] q0, q1, q2, output logic [1:0] q3);
always_comb begin
  q0 = data[7:0];
  q1 = data[3:0];
  q2 = data[idx +: 4];
  q3[1:0] = data[7:4];
end
endmodule
