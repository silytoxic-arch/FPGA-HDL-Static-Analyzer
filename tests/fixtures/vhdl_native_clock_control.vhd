-- Filename: vhdl_native_clock_control.vhd
-- Author: regression
-- Description: same clock in admitted control condition must be diagnosed
library ieee;
use ieee.std_logic_1164.all;
entity dut is
    port(clk, rst, din : in std_logic; dout : out std_logic);
end entity;
library ieee;
use ieee.std_logic_1164.all;
architecture rtl of dut is
begin
    process(clk)
    begin
        if rising_edge(clk) then
            if clk = '0' then
                dout <= din;
            end if;
        end if;
    end process;
end architecture;
