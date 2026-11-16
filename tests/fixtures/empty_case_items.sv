module empty_case_items(input logic [1:0] sel, output logic q);
always_comb begin
  q = 1'b0;
  case (sel)
    2'b00:;
    2'b01: begin end
    2'b10: begin : empty_named ; end
    default: q = 1'b1;
  endcase
  case (sel)
    2'b00: begin
      case (sel)
        2'b00:;
        default: q = 1'b0;
      endcase
    end
    default: q = 1'b1;
  endcase
  case (sel) default:; endcase
  case (sel) default: q = 1'b0; endcase
end
endmodule
