module power_on_register(input clk, dd, output reg qq);
    initial qq = 1'b0;
    always @(posedge clk) qq <= dd;
endmodule
