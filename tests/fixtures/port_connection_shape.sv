module count_child(input logic a, input logic b, output logic y);
assign y = a & b;
endmodule

module count_top(input logic a, output logic y);
count_child u_few(a, y);
count_child u_many(a, a, y, a);
count_child u_mixed(a, .b(a), .y(y));
endmodule
