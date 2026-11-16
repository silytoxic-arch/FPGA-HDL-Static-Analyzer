-- R-9-4 category-2 boundary: linked nested IF does not add a clock name.
entity r94_nested is
  port(clk1, clk2, d : in bit; q : out bit);
end entity;
architecture rtl of r94_nested is
begin
  p: process(clk1, clk2)
  begin
    if rising_edge(clk1) then
      if rising_edge(clk2) then
        q <= d;
      end if;
    end if;
  end process;
end architecture;
