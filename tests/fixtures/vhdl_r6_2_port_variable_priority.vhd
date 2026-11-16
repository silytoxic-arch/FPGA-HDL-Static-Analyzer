library ieee;
use ieee.numeric_std.all;

entity probe is
    port(value, other : in unsigned(7 downto 0); q : out bit);
end entity;

architecture rtl of probe is
begin
    process(all)
        variable value : unsigned(3 downto 0);
    begin
        if value = other then
            q <= '0';
        end if;
    end process;
end architecture;
