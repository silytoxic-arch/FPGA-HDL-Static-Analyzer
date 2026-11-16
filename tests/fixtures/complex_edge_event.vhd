entity complex_edge_event is
  port (
    clk     : in  bit;
    enable  : in  bit;
    d       : in  bit;
    q_bad   : out bit;
    q_good  : out bit
  );
end entity;

architecture rtl of complex_edge_event is
begin
  complex_clock : process(clk, enable)
  begin
    if rising_edge(clk and enable) then
      q_bad <= d;
    end if;
  end process;

  scalar_clock : process(clk)
  begin
    if rising_edge(clk) then
      if enable = '1' then
        q_good <= d;
      end if;
    end if;
  end process;
end architecture;
