module always_ff_nonedge_event(
  input  logic clk,
  input  logic d,
  output logic q_bad,
  output logic q_good
);
  always_ff @(clk)
    q_bad <= d;

  always_ff @(posedge clk)
    q_good <= d;
endmodule
