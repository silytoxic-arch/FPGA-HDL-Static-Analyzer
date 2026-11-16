module complex_edge_event(
  input  logic clk,
  input  logic enable,
  input  logic d,
  output logic q_bad,
  output logic q_good
);
  always_ff @(posedge (clk & enable))
    q_bad <= d;

  always_ff @(posedge clk)
    if (enable)
      q_good <= d;
endmodule
