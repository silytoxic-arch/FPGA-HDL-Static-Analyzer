-- R-9-4 positive: independent top-level IF relations are compared per process.
entity r94_top_level is
  port(clk1, clk2, d : in bit; q : out bit);
end entity;
architecture rtl of r94_top_level is
begin
  p: process(clk1, clk2)
  begin
    q <= d;
    if rising_edge(clk1) then
      q <= d;
    end if;
    if rising_edge(clk2) then
      q <= not d;
    end if;
  end process;
end architecture;
