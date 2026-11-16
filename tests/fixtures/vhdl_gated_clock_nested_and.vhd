entity gated_clock_nested_and_ent is
  port(clk_in,en_in,d_in: in bit; q_out: out bit);
end entity;
architecture rtl_arc of gated_clock_nested_and_ent is
  function wrap(x: bit) return bit is begin return x; end;
  signal derived_clk: bit;
begin
  derived_clk <= wrap(clk_in and en_in);
  p: process(derived_clk) begin
    if rising_edge(derived_clk) then q_out <= d_in; end if;
  end process;
end architecture;
