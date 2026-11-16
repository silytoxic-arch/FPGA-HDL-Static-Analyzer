-- File: vhdl_native_fsm.vhd
-- Author: FPGA Analyzer regression
-- Description: bounded native-backed encoding checks, not complete FSM equivalence.
library ieee;
use ieee.std_logic_1164.all;
entity native_fsm is
    port(clk:in std_logic);
end entity;
library ieee;
use ieee.std_logic_1164.all;
architecture rtl of native_fsm is
    type state_t is (idle, active, done);
    signal state:state_t;
    attribute fsm_encoding:string;
    attribute fsm_encoding of state:signal is "onehot";
begin
    process(clk) begin
        if rising_edge(clk) then
            case state is
                when idle=>state<=active;
                when active=>state<=done;
                when done=>state<=idle;
            end case;
        end if;
    end process;
end architecture;
