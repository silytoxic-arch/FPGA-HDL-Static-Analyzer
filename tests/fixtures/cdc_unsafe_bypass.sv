module cdc_unsafe_bypass(
  input logic clk_a, clk_b, d, enable,
  output logic source_a, synchronized, bypass
);
logic sync_ff1;
always_ff @(posedge clk_a) source_a <= d;
always_ff @(posedge clk_b) begin
  sync_ff1 <= source_a;
  synchronized <= sync_ff1;
  bypass <= source_a & enable;
end
endmodule
