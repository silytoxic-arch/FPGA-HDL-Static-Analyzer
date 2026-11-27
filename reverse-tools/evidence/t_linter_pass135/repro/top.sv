`include "flags.vh"
`ifdef FROM_HEADER
module active_owner;wire bad__;endmodule
`else
module inactive_owner;wire clean_name;endmodule
`endif
