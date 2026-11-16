-- R-9-4 category-4 boundary: late static branch cannot promote after extra edge.
entity r94_static_after is
  port(mode, arst, clk1, clk2, d : in bit; q : out bit);
end entity;
architecture rtl of r94_static_after is
begin
  p: process(mode, arst, clk1, clk2)
  begin
    if mode = '1' then
      q <= d;
    elsif rising_edge(clk1) then
      q <= d;
    elsif rising_edge(clk2) then
      q <= not d;
    elsif arst = '1' then
      q <= '0';
    end if;
  end process;
end architecture;
