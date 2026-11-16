module empty_conditional_branches(input logic a, b, output logic q);
always_comb begin
  q = 1'b0;
  if (a);
  if (b) begin end
  if (a) q = 1'b1; else;
  if (b) begin : empty_named ; end else begin q = 1'b1; end
  if (a) begin if (b); else q = 1'b1; end
  if (a) q = 1'b1; else if (b) q = 1'b0;
end
endmodule
