module shift_amounts(input logic [7:0] data, output logic [7:0] q);
always_comb begin
  q = data << 8;
  q = data >> 7;
  q = data[3:0] << 4;
  q = data[0] >>> 1;
  q = data[3:0] << 3;
  q <<= 8;
end
endmodule
