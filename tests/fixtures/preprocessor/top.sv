`include "common.vh"
`ifdef FEATURE_ENABLED
module feature_top;
    localparam int WIDTH = `COMMON_WIDTH;
endmodule
`else
module fallback_top;
endmodule
`endif
