module negative_shifts(input logic [7:0] data, output logic [7:0] q);
always_comb begin
  q = data << -1;
  q = data >> (-2);
  q >>>= -3;
  q = (data << -4) | data;
  q = data << 1;
  q = data << -0;
  q = data << (0 - 1);
  q = data << (-1 + 2);
end
endmodule
