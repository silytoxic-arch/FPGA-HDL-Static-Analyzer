module always_no_suspension(
  input logic a,
  output logic q0, q1, q2, q3, q4, q5, q6
);
always begin
  q0 = ~q0;
end
always q1 = a;
always @(a) q2 = a;
always begin
  wait (a);
  q3 = a;
end
always_comb q4 = a;
always_ff @(posedge a) q5 <= a;
initial q6 = a;
endmodule
