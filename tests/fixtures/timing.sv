module timing_fixture(
  input logic clk, input logic reset, input logic sync_reset, input logic d,
  output logic q_async, output logic q_sync
);
always_ff @(posedge clk or negedge reset) begin
  if (!reset) q_async <= 1'b0; else q_async <= d;
end
always_ff @(posedge clk) begin
  if (sync_reset) q_sync <= 1'b0; else q_sync <= d;
end
endmodule
