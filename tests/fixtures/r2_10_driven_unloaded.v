module probe(input wire a, output reg q);
  reg dead_signal;
  always @* begin
    q = a;
    dead_signal = a;
  end
endmodule
