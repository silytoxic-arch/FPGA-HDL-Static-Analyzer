module selected_target_width(input logic [2:0] idx, output logic [7:0] q0, q1, q2, q3);
always_comb begin
  q0[3:0] = 8'hff;
  q1[0] = 2'b10;
  q2[3:0] = 4'hf;
  q3[idx] = 2'b10;
end
endmodule
