architecture rtl of split_unsafe_fixture is
begin
  process(clk_a) begin
    if rising_edge(clk_a) then source_a <= d; end if;
  end process;
  process(clk_b) begin
    if rising_edge(clk_b) then destination_b <= source_a; end if;
  end process;
end architecture;
