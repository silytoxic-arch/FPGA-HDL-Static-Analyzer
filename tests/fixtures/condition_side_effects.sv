module condition_side_effects(output logic q);
integer a, b;
initial begin
  a = 0; b = 1; q = 1'b0;
  if (a = b) q = 1'b1;
  while ((a += 1) < 2) q = 1'b0;
  if (b++ > 0) q = 1'b1;
  if ((a <<= 1) != 0) q = 1'b0;
  if (a == b) q = 1'b1;
  if (a <= b) q = 1'b0;
  if (a != b) q = 1'b1;
  q = (a = b) ? 1'b1 : 1'b0;
  q = (a++) ? 1'b1 : 1'b0;
  q = (a == b) ? 1'b1 : 1'b0;
  q = b ? 1'b0 : (a += 1) ? 1'b1 : 1'b0;
end
endmodule
