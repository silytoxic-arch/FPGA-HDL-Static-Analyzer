library ieee;
use ieee.numeric_std.all;

entity probe is
    generic(mask : unsigned(7 downto 0) := (others => '0'));
    port(q : out bit);
end entity;

architecture rtl of probe is
    signal narrow : unsigned(3 downto 0);
begin
    process(all)
    begin
        if mask = narrow then
            q <= '0';
        end if;
    end process;
end architecture;
