library ieee;
use ieee.numeric_std.all;

entity probe is
    port(value : in unsigned(7 downto 0); q : out bit);
end entity;

architecture rtl of probe is
begin
    process(all)
        variable value : unsigned(3 downto 0);
        constant equal_port : unsigned(7 downto 0) := (others => '0');
    begin
        case value is
            when equal_port => q <= '0';
            when others => q <= '1';
        end case;
    end process;
end architecture;
