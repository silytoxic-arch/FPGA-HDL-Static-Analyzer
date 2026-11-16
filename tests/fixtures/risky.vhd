library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity risky_vhdl is
end entity;

architecture rtl of risky_vhdl is
begin
    process begin
        wait until rising_edge(clk);
    end process;
end architecture;
