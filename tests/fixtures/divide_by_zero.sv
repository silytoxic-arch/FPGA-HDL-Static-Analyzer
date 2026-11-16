module divide_by_zero(input logic [7:0] data, output logic [7:0] q);
always_comb begin
  q = data / 0;
  q = data % 4'b0000;
  q /= (0);
  q = data / ((0));
  q = data / 1;
  q = data / (0 + 1);
  if (data != 0) q = data % 3;
end
endmodule
