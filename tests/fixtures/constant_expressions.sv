module constant_expressions(input logic d, output logic q, r);
always_comb begin
  q = 1'b0;
  if (1'b0) q = d;
  while (1) begin
    q = d;
  end
  if (!(2 === 2)) q = d;
  if (d && 1'b0) q = d;
  if (1'b1 || d) q = d;
  if (d || 1'b0) q = d;
  if (d) q = 1'b0;
  q = 1'b0 ? d : 1'b0;
  q = d ? d : 1'b0;
  q = d ? 1'b0 : 1'b1 ? d : 1'b0;
end
assign r = 1'b1 ? d : 1'b0;
generate if (!(2 === 2)) begin : disabled end endgenerate
endmodule
