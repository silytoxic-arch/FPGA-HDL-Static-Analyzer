module gjb10157_child #(parameter WIDTH = 8) (
    input logic [WIDTH-1:0] data_i
);
endmodule

module gjb10157_defparam(input logic [3:0] data_i);
    gjb10157_child child(.data_i(data_i));
    defparam child.WIDTH = 4;
endmodule
