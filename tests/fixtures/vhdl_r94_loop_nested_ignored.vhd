-- R-9-4 boundary: an IF nested below LOOP is not a top-level relation root.
entity r94_loop is
  port(clk1, clk2, d : in bit; q : out bit);
end entity;
architecture rtl of r94_loop is
begin
  p: process(clk1, clk2)
  begin
    if rising_edge(clk1) then
      q <= d;
    end if;
    for i in 0 to 0 loop
      if rising_edge(clk2) then
        q <= not d;
      end if;
    end loop;
  end process;
end architecture;
