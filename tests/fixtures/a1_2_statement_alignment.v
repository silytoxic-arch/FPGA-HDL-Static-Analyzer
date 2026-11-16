module probe(input a, input b, output reg y);
always @* begin
  if (a) y = 0;
  else if (b) y = 1;
   else y = 0;
end
endmodule
