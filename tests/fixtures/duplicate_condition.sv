module duplicate_condition(
  input  logic condition,
  output logic q
);
  always_comb begin
    q = 1'b0;
    if (condition)
      q = 1'b1;
    else if ((condition))
      q = 1'b0;
  end
endmodule
