module empty_process(input logic a, output logic q);
always @(*) begin : empty_body
end
assign q = a;
endmodule
