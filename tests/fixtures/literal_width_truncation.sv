module literal_width_truncation(output logic [7:0] q0, q1, q2, q3, q4, q5, q6);
always_comb begin
  q0 = 16'h1234;
  q1 = ((12'd7));
  q2 = 8'hff;
  q3 = '0;
  q4 = -16'sd1;
  q5 = {8'h12};
end
assign q6 = 9'b1_0000_0000;
endmodule
