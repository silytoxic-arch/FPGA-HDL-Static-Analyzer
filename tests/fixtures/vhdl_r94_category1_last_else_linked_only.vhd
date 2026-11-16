-- R-9-4 boundary: only the last direct source IF linked below ELSE is used.
entity r94_last_linked is
  port(arst, clk1, clk2, d : in bit; q : out bit);
end entity;
architecture rtl of r94_last_linked is
begin
  p: process(arst, clk1, clk2)
  begin
    if arst = '1' then
      q <= '0';
    else
      if rising_edge(clk1) then
        q <= d;
      end if;
      if rising_edge(clk2) then
        q <= not d;
      end if;
    end if;
    if rising_edge(clk2) then
      q <= d;
    end if;
  end process;
end architecture;
