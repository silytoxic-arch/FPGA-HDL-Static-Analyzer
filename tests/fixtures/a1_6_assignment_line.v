module probe(input d, output reg q, output reg r);
always @* begin q=d; r=d; end
endmodule
