module baseline_new(input logic select, input logic data, output logic value);
always_comb begin
  casex (select)
    1'b0: value = data;
  endcase
end
endmodule
