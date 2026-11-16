module output_child(input logic a, output logic y, inout wire pad);
    assign y = a;
endmodule

module output_top(input logic a, output logic y, inout wire pad);
    output_child u_omitted(.a(a));
    output_child u_open(.a(a), .y(), .pad(pad));
    output_child u_positional(a, y);
    output_child u_ok(.a(a), .y(y), .pad(pad));
    output_child u_wildcard(.*);
endmodule
