entity self_assignment is
  port (clk : in bit; q : out bit);
end entity;

architecture rtl of self_assignment is
begin
  worker : process(clk)
  begin
    if clk'event and clk = '1' then
      q <= Q;
    end if;
  end process;
end architecture;
