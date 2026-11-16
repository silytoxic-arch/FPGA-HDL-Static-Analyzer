module unused_inputs(
    input  logic used,
    input  logic forwarded,
    input  logic unused,
    output logic q
);
    assign q = used;
    sink u_sink(.a(forwarded));
endmodule

module sink(input logic a);
    logic consumed;
    always_comb consumed = a;
endmodule
