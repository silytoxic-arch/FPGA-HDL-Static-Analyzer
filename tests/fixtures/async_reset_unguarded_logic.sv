module async_reset_unguarded_logic(
  input  logic clk,
  input  logic rst_n,
  input  logic enable,
  input  logic d,
  output logic q_bad,
  output logic q_good
);
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n)
      q_bad <= 1'b0;
    if (enable)
      q_bad <= d;
  end

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n)
      q_good <= 1'b0;
    else if (enable)
      q_good <= d;
  end
endmodule
