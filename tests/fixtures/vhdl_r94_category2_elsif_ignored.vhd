-- R-9-4 category-2 boundary: ELSIF edge is not a second category-2 clock.
entity r94_elsif is
  port(clk1, clk2, d : in bit; q : out bit);
end entity;
architecture rtl of r94_elsif is
begin
  p: process(clk1, clk2)
  begin
    if rising_edge(clk1) then
      q <= d;
    elsif rising_edge(clk2) then
      q <= not d;
    end if;
  end process;
end architecture;
