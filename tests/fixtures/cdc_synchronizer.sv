module cdc_synchronizer(
  input logic clk_a, clk_b, d,
  output logic source_a, synchronized
);
logic sync_ff1;
always_ff @(posedge clk_a) source_a <= d;
always_ff @(posedge clk_b) begin
  sync_ff1 <= source_a;
  synchronized <= sync_ff1;
end
endmodule
