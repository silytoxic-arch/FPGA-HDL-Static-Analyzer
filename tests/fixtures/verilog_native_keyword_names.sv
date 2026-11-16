// Filename: verilog_native_keyword_names.sv
// Author: regression
// Description: explicit configured native keyword predicates
module probe;
wire SIGNAL;
localparam MODULE = 1;
function integer TYPE;
  TYPE = 1;
endfunction
task CON;
endtask
wire SIGNAL_suffix;
wire COM10;
endmodule
