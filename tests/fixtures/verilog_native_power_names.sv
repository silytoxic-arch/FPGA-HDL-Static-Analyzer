// Filename: verilog_native_power_names.sv
// Author: FPGA Analyzer regression
// Description: exact, case-insensitive native power-name predicate.
module verilog_native_power_names;
    wire VDD, vSs, vdd_x;
    parameter VREF = 1;
    function integer GND;
        GND = 0;
    endfunction
endmodule
