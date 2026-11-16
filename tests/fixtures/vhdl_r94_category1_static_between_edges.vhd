-- R-9-4 category-1 positive: static branch between primary and extra edge.
entity r94_static_between is
  port(mode, arst, clk1, clk2, d : in bit; q : out bit);
end entity;
architecture rtl of r94_static_between is
begin
  p: process(mode, arst, clk1, clk2)
  begin
    if mode = '1' then
      q <= d;
    elsif rising_edge(clk1) then
      q <= d;
    elsif arst = '1' then
      q <= '0';
    elsif rising_edge(clk2) then
      q <= not d;
    end if;
  end process;
end architecture;
