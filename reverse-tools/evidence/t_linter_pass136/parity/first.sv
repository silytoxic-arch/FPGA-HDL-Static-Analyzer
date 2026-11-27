`include "flags.vh"
`define ID(x=default_name) x
`ifdef ACTIVE
module m;wire `JOIN(first,_name);endmodule
`elsif OTHER
module wrong;endmodule
`else
module wrong_else;endmodule
`endif
