library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
entity negative_shifts is
  port (data : in unsigned(7 downto 0); q : out unsigned(7 downto 0));
end entity;
architecture rtl of negative_shifts is begin
  process(all) begin
    q <= data sll -1;
    q <= data sra (-2);
    q <= shift_left(data, -3);
    q <= data srl 1;
    q <= data sll -0;
    q <= data sll (0 - 1);
    q <= shift_right(data, (-1 + 2));
  end process;
end architecture;
