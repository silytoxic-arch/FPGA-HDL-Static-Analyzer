entity legacy_vhdl_edge is
  port (
    clk_a : in  bit;
    clk_b : in  bit;
    d     : in  bit;
    q     : out bit
  );
end entity;

architecture rtl of legacy_vhdl_edge is
  signal source_a : bit;
begin
  source_domain : process(clk_a)
  begin
    if CLK_A'event and clk_a = '1' then
      source_a <= d;
    end if;
  end process;

  destination_domain : process(clk_b)
  begin
    if '0' = CLK_B and clk_b'event then
      q <= source_a;
    end if;
  end process;
end architecture;
