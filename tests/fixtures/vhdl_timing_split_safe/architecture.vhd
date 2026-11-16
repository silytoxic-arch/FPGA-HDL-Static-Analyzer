architecture rtl of split_timing_fixture is
  signal sync_ff1 : std_logic;
begin
  process(clk_a) begin
    if rising_edge(clk_a) then source_a <= d; end if;
  end process;
  process(clk_b) begin
    if rising_edge(clk_b) then
      sync_ff1 <= source_a;
      synchronized <= sync_ff1;
    end if;
  end process;
end architecture;
