module multiple_case_defaults(input logic sel, output logic q);
always_comb begin
  q = 1'b0;
  case (sel)
    1'b0: q = 1'b0;
    default: q = 1'b1;
    default: q = 1'b0;
  endcase
end
endmodule
