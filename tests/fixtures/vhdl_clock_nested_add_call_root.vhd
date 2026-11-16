library ieee;
use ieee.std_logic_1164.all;
entity clock_nested_add_call_root_ent is
  port(clk_in,en_in,d_in: in std_logic; q_out: out std_logic);
end entity;
architecture rtl_arc of clock_nested_add_call_root_ent is
  function "+"(l,r: std_logic) return std_logic is begin return l; end;
  function wrap(x: std_logic) return std_logic is begin return x; end;
  signal derived_clk: std_logic;
begin
  derived_clk <= wrap(clk_in + en_in);
  p: process(derived_clk) begin
    if rising_edge(derived_clk) then q_out <= d_in; end if;
  end process;
end architecture;
