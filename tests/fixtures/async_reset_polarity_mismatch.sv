module async_reset_polarity_mismatch(
  input logic clk, rst_n, d,
  output logic q_bad, q_good
);
always_ff @(posedge clk or negedge rst_n)
  if (rst_n) q_bad <= 1'b0; else q_bad <= d;
always_ff @(posedge clk or negedge rst_n)
  if (!rst_n) q_good <= 1'b0; else q_good <= d;
endmodule
