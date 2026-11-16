module malformed(input wire a;
    always @(*) begin
        if (a) begin
            $display("broken");
        end
    end
