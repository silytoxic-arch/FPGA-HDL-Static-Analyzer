-- R-9-4 category-1 positive: ELSE-linked clock plus a different top-level clock.
entity r94_else_linked is
  port(arst, clk1, clk2, d : in bit; q : out bit);
end entity;
architecture rtl of r94_else_linked is
begin
  p: process(arst, clk1, clk2)
  begin
    if arst = '1' then
      q <= '0';
    else
      q <= d;
      if rising_edge(clk1) then
        q <= d;
      end if;
    end if;
    if rising_edge(clk2) then
      q <= not d;
    end if;
  end process;
end architecture;
