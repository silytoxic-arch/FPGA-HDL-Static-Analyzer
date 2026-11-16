library ieee;
use ieee.std_logic_1164.all;

entity gjb9765_case_buffer is
  port (
    sel : in std_logic;
    q   : buffer std_logic
  );
end entity;

architecture rtl of gjb9765_case_buffer is
begin
  process(all)
  begin
    case sel is
      when '0' => q <= '0';
      when '1' => q <= '1';
    end case;
  end process;
end architecture;
