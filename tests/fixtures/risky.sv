module risky(input logic clk, input logic d, output logic q);
    always_ff @(posedge clk) begin
        q = d;
    end

    always_comb begin
        q <= d;
    end
endmodule
