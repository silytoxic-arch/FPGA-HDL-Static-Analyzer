library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
entity probe is
  port(a,b: in signed(0 downto 0); c: inout std_logic);
end;
architecture rtl of probe is
  signal logic_en: signed(0 downto 0);
begin
  logic_en <= a + b;
  c <= b(0) when logic_en(0)='1' else 'Z';
end architecture;
