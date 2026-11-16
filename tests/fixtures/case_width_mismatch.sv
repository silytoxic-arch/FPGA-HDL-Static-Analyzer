module case_width_mismatch(input logic [3:0] sel, output logic q);
always_comb begin
  q = 1'b0;
  case (sel)
    3'b000: q = 1'b0;
    4'b0001: q = 1'b1;
    5'h02: q = 1'b0;
    0: q = 1'b1;
    default: q = 1'b0;
  endcase
  case (sel[2:0])
    2'b00: q = 1'b0;
    3'b001: q = 1'b1;
    default: q = 1'b0;
  endcase
  case (sel[0])
    2'b00: q = 1'b0;
    1'b1: q = 1'b1;
    default: q = 1'b0;
  endcase
end
endmodule
