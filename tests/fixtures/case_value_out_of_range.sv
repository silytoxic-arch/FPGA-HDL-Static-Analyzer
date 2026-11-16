module case_value_out_of_range #(parameter P = 7)(input logic [1:0] sel, output logic q);
always_comb begin
  case (sel)
    0: q = 1'b0;
    3: q = 1'b1;
    4: q = 1'b0;
    5, 2'd2: q = 1'b1;
    -1: q = 1'b0;
    P: q = 1'b1;
    default: q = 1'b0;
  endcase
  case (sel[0])
    0: q = 1'b0;
    2: q = 1'b1;
    default: q = 1'b0;
  endcase
  case (sel[1:0])
    4: q = 1'b0;
    default: q = 1'b1;
  endcase
end
endmodule
