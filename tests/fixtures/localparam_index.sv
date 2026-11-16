module localparam_index(input [3:1] data, output wire bad, good);
localparam integer BAD_INDEX = 4;
localparam integer GOOD_INDEX = 3;
assign bad = data[BAD_INDEX];
assign good = data[GOOD_INDEX];
endmodule
