module tokenized_cases(input logic [1:0] sel, output logic q);
always_comb begin
    q = 1'b0;
    case (sel) 2'b00: q = 1'b1; endcase
    casex (sel) default: q = 1'b0; endcase
end
endmodule
